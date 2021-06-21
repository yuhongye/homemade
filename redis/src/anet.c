#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>

#include "anet.h"

static void anetSetError(char *err, const char *fmt, ...) {
    va_list ap;
    if (err == NULL) {
        return;
    }

    va_start(ap, fmt);
    vsnprintf(err, ANET_ERR_LEN, fmt, ap);
    va_end(ap);
}

/***************************************************
 * 下面的几个函数设置了socket的属性：
 * 1. nonblock
 * 2. tcp_nodelay
 * 3. so_sndbuf
 * 4. keep_alive
 */

int anetNonBlock(char *err, int fd) {
    int flags;

    /**
     * Set the socket nonblocking
     */
    if ((flags = fcntl(fd, F_GETFL)) == -1) {
        anetSetError(err, "fcntl(F_GETFL: %s\n", strerror(errno));
        return ANET_ERR;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        anetSetError(err, "fcntl(F_SETFL,O_NONBLOCK): %s\n", strerror(errno));
        return ANET_ERR;
    }

    return ANET_OK;
}

int anetTcpNoDelay(char *err, int fd) {
    int yes = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes)) == -1) {
        anetSetError(err, "setsockopt TCP_NODELAY: %s\n", strerror(errno));
        return ANET_ERR;
    }
    return  ANET_OK;
}

int anetSetSendBuffer(char *err, int fd, int buffsize) {
    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &buffsize, sizeof(buffsize)) == -1) {
        anetSetError(err, "setsockopt SO_SNDBUF: %s\n", strerror(errno));
        return ANET_ERR;
    }
    return ANET_OK;
}

int anetTcpKeepAlive(char *err, int fd) {
    int yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes)) == -1) {
        anetSetError(err, "setsockopt SO_KEEPALIVE: %s\n", strerror(errno));
        return ANET_ERR;
    }
    return ANET_OK;
}


/**
 * 将 host 解析成 ip 格式保存早 ipbuf 中
 */
int anetResolve(char *err, char *host, char *ipbuf) {
    // ipv4 的地址
    struct sockaddr_in sa;
    // ipv4
    sa.sin_family = AF_INET;
    /**
     * struct in_addr {
     *  uint32_t s_addr; // 32位的ipv4地址
     * }
     * 
     * struct sockaddr_in {
     *  sa_family_t sin_family;
     *  in_port_t sin_port; // 16bit port
     *  struct in_addr sin_addr; // ip 地址
     * }
     * 
     */
    if (inet_aton(host, &sa.sin_addr) == 0) { // 返回 0 表示失败
        struct hostent *he =gethostbyname(host);
        if (he == NULL) {
            anetSetError(err, "can't resolve: %s\n", host);
            return ANET_ERR;
        }
        memcpy(&sa.sin_addr, he->h_addr, sizeof(struct in_addr));
    }
    strcpy(ipbuf, inet_ntoa(sa.sin_addr));
    return ANET_OK;
}

/** 创建 tcp 连接，返回的是 socket fd */
#define ANET_CONNECT_NONE 0
#define ANET_CONNECT_NONBLOCK 1
static int anetTcpGenericConnect(char *err, char *addr, int port, int flags) {
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == -1) {
        anetSetError(err, "creating socket: %s\n", strerror(errno));
        return ANET_ERR;
    }

    int on = -1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    if (inet_aton(addr, &sa.sin_addr) == 0) { // 返回 0 表示失败
        struct hostent *he = gethostbyname(addr);
        if (he == NULL) {
            anetSetError(err, "can't resolve: %s\n", addr);
            close(s);
            return ANET_ERR;
        }
        memcpy(&sa.sin_addr, he->h_addr, sizeof(struct in_addr));
    }

    if ((flags & ANET_CONNECT_NONBLOCK) != 0 && anetNonBlock(err, s) != ANET_OK) {
        return ANET_ERR;
    }

    if (connect(s, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
        if (errno == EINPROGRESS && (flags & ANET_CONNECT_NONBLOCK) != 0) {
            return s;
        }

        anetSetError(err, "connect: %s\n", strerror(errno));
        close(s);
        return ANET_ERR;
    }

    return s;
}

int anetTcpConnect(char *err, char *addr, int port) {
    return anetTcpGenericConnect(err, addr, port, ANET_CONNECT_NONE);
}

int anetTcpNonBlockConnect(char *err, char *addr, int port) {
    return anetTcpGenericConnect(err, addr, port, ANET_CONNECT_NONBLOCK);
}

/**
 * @param fd socket
 * @param buf dest
 * @param count 读取的字节数
 * @return 读取的字节数, or -1 if failed.
 */
int anetRead(int fd, void *buf, int count) {
    int nread, totlen = 0;
    while (totlen != count) {
        nread = read(fd, buf, count - totlen);
        if (nread == 0) {
            return totlen;
        } else if (nread == -1) {
            return -1;
        }

        totlen += nread;
        buf += nread;
    }
    return totlen;
}

/**
 * @return 写入的字节数，or -1 if failed.
 */
int anetWrite(int fd, void *buf, int count) {
    int nwritten, totlen = 0;
    while(totlen != count) {
        nwritten = write(fd, buf, count - totlen);
        if (nwritten == 0) {
            return totlen;
        } else if (nwritten == -1) {
            return -1;
        }

        totlen += nwritten;
        buf += nwritten;
    }
    return totlen;
}

/**
 * @return socket fd, or -1 if error
 */
int anetTcpServer(char *err, int port, char *bindaddr) {
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == -1) {
        anetSetError(err, "socket: %s\n", strerror(errno));
        return ANET_ERR;
    }

    int on = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) {
        anetSetError(err, "setsockopt SO_REUSEDADDR: %s\n", strerror(errno));
        close(s);
        return ANET_ERR;
    }

    struct sockaddr_in sa; // ipv4 address
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bindaddr != NULL && inet_aton(bindaddr, &sa.sin_addr) == 0) {
        anetSetError(err, "Invalid bind address\n");
        close(s);
        return ANET_ERR;
    }

    if (bind(s, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
        anetSetError(err, "bind: %s\n", strerror(errno));
        close(s);
        return ANET_ERR;
    }

    if (listen(s, 32) == -1) {
        anetSetError(err, "listen: %s\n", strerror(errno));
        close(s);
        return ANET_ERR;
    }
    return s;
}

/**
 * @param ip 获取 client socket 后提取其中的 ip 保存到该参数中
 * @param port 获取 client socket 后提取其中的 port 保存到该参数中
 * @return client socket
 */
int anetAccept(char *err, int servsock, char *ip, int *port) {
    int fd;
    struct sockaddr_in sa;
    unsigned int saLen;
    while (true) {
        saLen = sizeof(sa);
        fd = accept(servsock, (struct sockaddr *)&sa, &saLen);
        if (fd == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                anetSetError(err, "accept: %s\n", strerror(errno));
                return ANET_ERR;
            }
        }
        break;
    }
    if (ip != NULL) {
        strcpy(ip, inet_ntoa(sa.sin_addr));
    }
    if (port != NULL) {
        *port = ntohs(sa.sin_port);
    }
    return fd;
}

/********************************** test *************************/
void server() {
    char err[256];
    int server = anetTcpServer(err, 1234, NULL);
    int client = anetAccept(err, server, NULL, NULL);
    char buf[9];
    anetRead(client, buf, 8);
    buf[8] = '\0';
    printf("Read from client: %s\n", buf);
    anetWrite(client, buf, 8);
    close(client);
    close(server);
}

void client() {
    char err[256]; 
    int fd = anetTcpConnect(err, "127.0.0.1", 1234);
    anetWrite(fd, "12345678", 8);
    char buf[9];
    anetRead(fd, buf, 8);
    buf[8] = '\0';
    printf("Read from server: %s\n", buf);
    close(fd);
}

int main3(int argc, char *argv[]) {
    setvbuf (stdout, NULL, _IONBF, 0);
    if (strcmp(argv[1], "server") == 0) {
        printf("Server start...\n");
        server();
    } else {
        printf("Client start ...\n");
        client();
    }

    return 0;
}
