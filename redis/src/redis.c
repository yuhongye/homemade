/*
 * Copyright (c) 2006-2009, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#define REDIS_VERSION "0.07"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "ae.h"     /* Event driven programming library */
#include "sds.h"    /* Dynamic safe strings */
#include "anet.h"   /* Networking the easy way */
#include "dict.h"   /* Hash tables */
#include "adlist.h" /* Linked lists */
#include "zmalloc.h" /* total memory usage aware version of malloc/free */

#define REDIS_OK   0
#define REDIS_ERR -1

/** Static server configuration */
#define REDIS_SERVERPORT       6379
#define REDIS_MAXIDLETIME      (60 * 5) // default client timeout
#define REDIS_QUERYBUF_LEN     1024
#define REDIS_LOADBUF_LEN      1024
#define REDIS_MAX_ARGS         16
#define REDIS_DEFULT_DBNUM     16
#define REDIS_CONFIGLINE_MAX   1024
#define REDIS_OBJFREELIST_MAX  1000000 // Max number of object to cache
#define REDIS_MAX_SYNC_TIME    60      // Slave can't take more to sync

/** Hash table parameters */
#define REDIS_HT_MINFILL       10    // Minimal hash table fill 10%
#define REDIS_HT_MINSLOTS      16384 // Never resize the HT under this

/** Command flags */
#define REDIS_CMD_BULK         1
#define REDIS_CMD_INLINE       2

/** Object types */
#define REDIS_STRING           0
#define REDIS_LIST             1
#define REDIS_SET              2
#define REDIS_HASH             3
#define REDIS_SELECTDB         254
#define REDIS_EOF              255

/** Server replication state */
#define REDIS_REPL_NONE        0    // No active replication
#define REDIS_REPL_CONNECT     1    // Must connect to master
#define REDIS_REPL_CONNECTED   2    // Connected to master

/** Client flags */
#define REDIS_CLOSE            1     // This client connection should be closed ASAP
#define REDIS_SLAVE            2     // This client is a slave server
#define REDIS_MASTER           4     // This client is a master server
#define isSlave(flags) (((flags) & REDIS_SLAVE) != 0)
#define isMaster(flags) (((flags) & REDIS_MASTER) != 0)

/** Sort operations */
#define REDIS_SORT_GET         0
#define REDIS_SORT_DEL         1
#define REDIS_SORT_INCR        2
#define REDIS_SORT_DECR        3
#define REDIS_SORT_ASC         4
#define REDIS_SORT_DESC        5
#define REDIS_SORTKEY_MAX      1024


/** Log levels */
#define REDIS_DEBUG            0
#define REDIS_NOTICE           1
#define REDIS_WARNING          2

#define REDIS_NOTUSED(V) ((void) V)

/*=========================== 数据结构定义 ======================== */
typedef struct redisObject {
    int type;
    void *ptr;
    int refcount;
} robj;

/**
 * client state. Clients are taken in a linked list
 */
typedef struct redisClient {
    int fd;
    dict *dict;
    int dictid;
    sds querybuf;
    robj *argv[REDIS_MAX_ARGS];
    int argc;
    int bulklen; // bulk read len. -1 if not in bulk read mode;
    list *reply;
    int sentlen;
    time_t lastinteraction; // time of the last interaction, used for timeout
    int flags; // REDIS_CLOSE | REDIS_SLAVE
    int slaveseldb; // slave selected db, if this client is a slave
} redisClient;

/**
 * 多长时间内执行多少次更新操作需要执行保存
 */
struct saveparam {
    time_t seconds;
    int changes;
};

/** 全局的 server state */
struct redisServer {
    int port;
    int fd;
    dict **dict;
    long long dirty; // 上次保存后的修改次数
    list *clients;
    list *slaves;
    char neterr[ANET_ERR_LEN];
    aeEventLoop *el;
    int cronloops; // cron function 的运行次数
    list *objfreelist; // free object, 避免 malloc()
    time_t lastsave;  // 上次保存的时间，unix time
    int usedmemory;  // 占用的内存，MB

    /* Fields used only for stats */
    time_t stat_starttime; // server start time
    long long stat_numcommands; // number of processed commands
    long long stat_numconnections; // number of connections received

    /* Configuration */
    int verbosity;
    int glueoutputbuf;
    int maxidletime;
    int dbnum;
    bool daemonize;
    bool bgsaveinprogress;
    struct saveparam *saveparams;

    int saveparamslen;
    char *logfile;
    char *bindaddr;
    char *dbfilename;

    /* Replication related */
    bool isslave;
    char *masterhost;
    int masterport;
    redisClient *master;
    int replstate;

    /* Sort parameters */
    int sort_desc;
    int sort_alpha;
    int sort_bypattern;
};

/** 处理具体命令的函数 */
typedef void redisCommandProc(redisClient *c);

struct redisCommand {
    char *name;
    redisCommandProc *proc;
    int arity;
    int flags;
};

typedef struct _redisSortObject {
    robj *obj;
    union {
        double score;
        robj *cmpobj;
    } u;
} redisSortObject;

typedef struct _redisSortOperation {
    int type;
    robj *pattern;
} redisSortOperation;

struct sharedObjectsStruct {
    robj *crlf, *ok, *err, *zerobulk, *nil, *zero, *one, *pong, *space,
    *minus1, *minus2, *minus3, *minus4,
    *wrongtypeerr, *nokeyerr, *wrongtypeerrbulk, *nokeyerrbulk,
    *syntaxerr, *syntaxerrbulk,
    *select0, *select1, *select2, *select3, *select4,
    *select5, *select6, *select7, *select8, *select9;
} shared;

/*================================ Prototypes =============================== */

static void freeStringObject(robj *o);
static void freeListObject(robj *o);
static void freeSetObject(robj *o);
static void decrRefCount(void *o);
static robj *createObject(int type, void *ptr);
static void freeClient(redisClient *c);
static int loadDb(char *filename);
static void addReply(redisClient *c, robj *obj);
static void addReplySds(redisClient *c, sds s);
static void incrRefCount(robj *o);
static int saveDbBackground(char *filename);
static robj *createStringObject(char *ptr, size_t len);
static void replicationFeedSlaves(struct redisCommand *cmd, int dictid, robj **argv, int argc);
static int syncWithMaster(void);

static void pingCommand(redisClient *c);
static void echoCommand(redisClient *c);
static void setCommand(redisClient *c);
static void setnxCommand(redisClient *c);
static void getCommand(redisClient *c);
static void delCommand(redisClient *c);
static void existsCommand(redisClient *c);
static void incrCommand(redisClient *c);
static void decrCommand(redisClient *c);
static void incrbyCommand(redisClient *c);
static void decrbyCommand(redisClient *c);
static void selectCommand(redisClient *c);
static void randomkeyCommand(redisClient *c);
static void keysCommand(redisClient *c);
static void dbsizeCommand(redisClient *c);
static void lastsaveCommand(redisClient *c);
static void saveCommand(redisClient *c);
static void bgsaveCommand(redisClient *c);
static void shutdownCommand(redisClient *c);
static void moveCommand(redisClient *c);
static void renameCommand(redisClient *c);
static void renamenxCommand(redisClient *c);
static void lpushCommand(redisClient *c);
static void rpushCommand(redisClient *c);
static void lpopCommand(redisClient *c);
static void rpopCommand(redisClient *c);
static void llenCommand(redisClient *c);
static void lindexCommand(redisClient *c);
static void lrangeCommand(redisClient *c);
static void ltrimCommand(redisClient *c);
static void typeCommand(redisClient *c);
static void lsetCommand(redisClient *c);
static void saddCommand(redisClient *c);
static void sremCommand(redisClient *c);
static void sismemberCommand(redisClient *c);
static void scardCommand(redisClient *c);
static void sinterCommand(redisClient *c);
static void sinterstoreCommand(redisClient *c);
static void syncCommand(redisClient *c);
static void flushdbCommand(redisClient *c);
static void flushallCommand(redisClient *c);
static void sortCommand(redisClient *c);
static void lremCommand(redisClient *c);
static void infoCommand(redisClient *c);

/*=========================== 全局变量 ===========================*/

static struct redisServer server;
static struct redisCommand cmdTable[] = {
    {"get",        getCommand,          2, REDIS_CMD_INLINE},
    {"set",        setCommand,          3, REDIS_CMD_BULK},
    {"setnx",      setnxCommand,        3, REDIS_CMD_BULK},
    {"del",        delCommand,          2, REDIS_CMD_INLINE},
    {"exists",     existsCommand,       2, REDIS_CMD_INLINE},
    {"incr",       incrCommand,         2, REDIS_CMD_INLINE},
    {"decr",       decrCommand,         2, REDIS_CMD_INLINE},
    {"rpush",      rpushCommand,        3, REDIS_CMD_BULK},
    {"lpush",      lpushCommand,        3, REDIS_CMD_BULK},
    {"rpop",       rpopCommand,         2, REDIS_CMD_INLINE},
    {"lpop",       lpopCommand,         2, REDIS_CMD_INLINE},
    {"llen",       llenCommand,         2, REDIS_CMD_INLINE},
    {"lindex",     lindexCommand,       3, REDIS_CMD_INLINE},
    {"lset",       lsetCommand,         4, REDIS_CMD_BULK},
    {"lrange",     lrangeCommand,       4, REDIS_CMD_INLINE},
    {"ltrim",      ltrimCommand,        4, REDIS_CMD_INLINE},
    {"lrem",       lremCommand,         4, REDIS_CMD_BULK},
    {"sadd",       saddCommand,         3, REDIS_CMD_BULK},
    {"srem",       sremCommand,         3, REDIS_CMD_BULK},
    {"sismember",  sismemberCommand,    3, REDIS_CMD_BULK},
    {"scard",      scardCommand,        2, REDIS_CMD_INLINE},
    {"sinter",     sinterCommand,      -2, REDIS_CMD_INLINE},
    {"sinterstore",sinterstoreCommand, -3, REDIS_CMD_INLINE},
    {"smembers",   sinterCommand,       2, REDIS_CMD_INLINE},
    {"incrby",     incrbyCommand,       3, REDIS_CMD_INLINE},
    {"decrby",     decrbyCommand,       3, REDIS_CMD_INLINE},
    {"randomkey",  randomkeyCommand,    1, REDIS_CMD_INLINE},
    {"select",     selectCommand,       2, REDIS_CMD_INLINE},
    {"move",       moveCommand,         3, REDIS_CMD_INLINE},
    {"rename",     renameCommand,       3, REDIS_CMD_INLINE},
    {"renamenx",   renamenxCommand,     3, REDIS_CMD_INLINE},
    {"keys",       keysCommand,         2, REDIS_CMD_INLINE},
    {"dbsize",     dbsizeCommand,       1, REDIS_CMD_INLINE},
    {"ping",       pingCommand,         1, REDIS_CMD_INLINE},
    {"echo",       echoCommand,         2, REDIS_CMD_BULK},
    {"save",       saveCommand,         1, REDIS_CMD_INLINE},
    {"bgsave",     bgsaveCommand,       1, REDIS_CMD_INLINE},
    {"shutdown",   shutdownCommand,     1, REDIS_CMD_INLINE},
    {"lastsave",   lastsaveCommand,     1, REDIS_CMD_INLINE},
    {"type",       typeCommand,         2, REDIS_CMD_INLINE},
    {"sync",       syncCommand,         1, REDIS_CMD_INLINE},
    {"flushdb",    flushdbCommand,      1, REDIS_CMD_INLINE},
    {"flushall",   flushallCommand,     1, REDIS_CMD_INLINE},
    {"sort",       sortCommand,        -2, REDIS_CMD_INLINE},
    {"info",       infoCommand,         1, REDIS_CMD_INLINE},
    {NULL,         NULL,                0, 0}
};

static void oom(const char *msg) {
    fprintf(stderr, "%s: Out Of Memory\n", msg);
    fflush(stderr);
    sleep(1);
    abort();
}

static void createSharedObjects(void) {
    shared.crlf = createObject(REDIS_STRING,sdsnew("\r\n"));
    shared.ok = createObject(REDIS_STRING,sdsnew("+OK\r\n"));
    shared.err = createObject(REDIS_STRING,sdsnew("-ERR\r\n"));
    shared.zerobulk = createObject(REDIS_STRING,sdsnew("0\r\n\r\n"));
    shared.nil = createObject(REDIS_STRING,sdsnew("nil\r\n"));
    shared.zero = createObject(REDIS_STRING,sdsnew("0\r\n"));
    shared.one = createObject(REDIS_STRING,sdsnew("1\r\n"));
    /* no such key */
    shared.minus1 = createObject(REDIS_STRING,sdsnew("-1\r\n"));
    /* operation against key holding a value of the wrong type */
    shared.minus2 = createObject(REDIS_STRING,sdsnew("-2\r\n"));
    /* src and dest objects are the same */
    shared.minus3 = createObject(REDIS_STRING,sdsnew("-3\r\n"));
    /* out of range argument */
    shared.minus4 = createObject(REDIS_STRING,sdsnew("-4\r\n"));
    shared.pong = createObject(REDIS_STRING,sdsnew("+PONG\r\n"));
    shared.wrongtypeerr = createObject(REDIS_STRING,sdsnew(
        "-ERR Operation against a key holding the wrong kind of value\r\n"));
    shared.wrongtypeerrbulk = createObject(REDIS_STRING,sdscatprintf(sdsempty(),"%d\r\n%s",-sdslen(shared.wrongtypeerr->ptr)+2,shared.wrongtypeerr->ptr));
    shared.nokeyerr = createObject(REDIS_STRING,sdsnew(
        "-ERR no such key\r\n"));
    shared.nokeyerrbulk = createObject(REDIS_STRING,sdscatprintf(sdsempty(),"%d\r\n%s",-sdslen(shared.nokeyerr->ptr)+2,shared.nokeyerr->ptr));
    shared.syntaxerr = createObject(REDIS_STRING,sdsnew(
        "-ERR syntax error\r\n"));
    shared.syntaxerrbulk = createObject(REDIS_STRING,sdscatprintf(sdsempty(),"%d\r\n%s",-sdslen(shared.syntaxerr->ptr)+2,shared.syntaxerr->ptr));
    shared.space = createObject(REDIS_STRING,sdsnew(" "));
    shared.select0 = createStringObject("select 0\r\n",10);
    shared.select1 = createStringObject("select 1\r\n",10);
    shared.select2 = createStringObject("select 2\r\n",10);
    shared.select3 = createStringObject("select 3\r\n",10);
    shared.select4 = createStringObject("select 4\r\n",10);
    shared.select5 = createStringObject("select 5\r\n",10);
    shared.select6 = createStringObject("select 6\r\n",10);
    shared.select7 = createStringObject("select 7\r\n",10);
    shared.select8 = createStringObject("select 8\r\n",10);
    shared.select9 = createStringObject("select 9\r\n",10);
}

/* ======================= Hash table type implementation =================== */
/**
 * key: sds
 * value: redis object
 */

static bool sdsDictKeyCompare(void *privdata, const void *key1, const void *key2) {
    DICT_NOTUSED(privdata);
    int l1 = sdslen((sds) key1);
    int l2 = sdslen((sds) key2);
    if (l1 != l2) {
        return false;
    }
    return memcmp(key1, key2, l1) == 0;
}

static dictRedisObjectDestructor(void *privdata, void *val) {
    DICT_NOTUSED(privdata);
    decrRefCount(val);
}

static int dictSdsKeyCompare(void *privdata, const void *key1, const void *key2) {
    const robj *o1 = key1;
    const robj *o2 = key2;
    return sdsDictKeyCompare(privdata, o1->ptr, o2->ptr);
}

static unsigned int dictSdsHash(const void *key) {
    const robj *o = key;
    return dictGenHashFunction(o->ptr, sdslen((sds)o->ptr));
}

/**
 * set 使用 hashtable 来实现， value is null
 */
static dictType setDictType = {
    dictSdsHash,                 // hash function
    NULL,                        // key dup
    NULL,                        // val dup
    dictSdsKeyCompare,           // key compare
    dictRedisObjectDestructor,   // key destructor
    NULL                         // val destructor
};

static dictType hashDictType = {
    dictSdsHash,                // hash function
    NULL,                       // key dup
    NULL,                       // val dup
    dictSdsKeyCompare,          // key compare
    dictRedisObjectDestructor,  // key destructor
    dictRedisObjectDestructor   // val destructor
};

/* ====================== Redis objects implmentation ============== */
static robj *createObject(int type, void *ptr) {
    robj *o;
    if (listLength(server.objfreelist) > 0) {
        listNode *head = listFirst(server.objfreelist);
        o = listNodeValue(head);
        listDelNode(server.objfreelist, head);
    } else {
        o = zmalloc(sizeof(*o));
        if (o == NULL) {
            oom("createObject");
        }
    }

    o->type = type;
    o->ptr = ptr;
    o->refcount = 1;
    return o;
}

static robj *createStringObject(char *ptr, size_t len) {
    return createObject(REDIS_STRING, sdsnewlen(ptr, len));
}

static robj *createListObject(void) {
    list *list = listCreate();
    if (list == NULL) {
        oom("listCreate");
    }
    listSetFreeMethod(list, decrRefCount);
    return createObject(REDIS_LIST, list);
}

static robj *createSetObject(void) {
    dict *d = dictCreate(&setDictType, NULL);
    if (d == NULL) {
        oom("dictCreate");
    }
    return createObject(REDIS_SET, d);
}

#if 0
static robj *createHashObject(void) {
    dict *d = dictCreate(&hashDictType,NULL);
    if (!d) oom("dictCreate");
    return createObject(REDIS_SET,d);
}
#endif

static void freeStringObject(robj *o) {
    sdsfree(o->ptr);
}

static void freeListObject(robj *o) {
    listRelease((list*) o->ptr);
}

static void freeSetObject(robj *o) {
    dictRelease((dict *) o->ptr);
}

static void freeHashObject(robj *o) {
    dictRelease((dict *) o->ptr);
}

static void incrRefCount(robj *o) {
    o->refcount++;
}

static void decrRefCount(void *obj) {
    robj *o = obj;
    if (--(o->refcount) == 0) {
        switch (o->type) {
        case REDIS_STRING:
            freeStringObject(o);
            break;
        case REDIS_LIST:
            freeListObject(o);
            break;
        case REDIS_SET:
            freeSetObject(o);
            break;
        case REDIS_HASH:
            freeHashObject(o);
            break;
        default:
            assert(false);
            break;
        }

        if (listLength(server.objfreelist) > REDIS_OBJFREELIST_MAX ||
            listAddNodeHead(server.objfreelist, o) == NULL) {
            zfree(o);
        }
    }
}

/* ===================== Replication ========================= */

/**
 * Send the whole output buffer syncronously to the slave
 */
static int flushClientOutput(redisClient *c) {
    time_t start = time(NULL);
    while (listLength(c->reply) > 0) {
        if (time(NULL) - start > 5) {
            return REDIS_ERR; // 5 seconds timeout
        }
        int retval = aeWait(c->fd, AE_WRITABLE, 1000);
        if (retval == -1) {
            return REDIS_ERR;
        } else if (isWritable(retval)) {
            sendReplyToClient(NULL, c->fd, c, AE_WRITABLE);
        }
    }
    return REDIS_OK;
}


/**
 * 将 ptr 中的 size 个字节在 timeout 的时间内写入到 fd 中
 */
static int syncWrite(int fd, void *ptr, ssize_t size, int timeout) {
    ssize_t ret = size;
    ssize_t nwritten;
    time_t start = time(NULL);

    timeout++;
    while (size != 0) {
        if ((aeWait(fd, AE_WRITABLE, 1000) & AE_WRITABLE) != 0) {
            nwritten = write(fd, ptr, size);
            if (nwritten == -1) {
                return -1;
            }
            ptr += nwritten;
            size -= nwritten;
        }
        if ((time(NULL) - start) > timeout) {
            errno = ETIMEDOUT;
            return -1;
        }
    }
    return ret;
}

static int syncRead(int fd, void *ptr, ssize_t size, int timeout) {
    ssize_t nread, totread = 0;
    time_t start = time(NULL);
    timeout++;

    while (size != 0) {
        if (aeWait(fd, AE_READABLE, 1000) & AE_READABLE) {
            nread = read(fd, ptr, size);
            if (nread == -1) {
                return -1;
            }
            ptr += nread;
            size -= nread;
            totread += nread;
        }
        if ((time(NULL) - start) > timeout) {
            errno = ETIMEDOUT;
            return -1;
        }
    }
    return totread;
}

/**
 * 读一行，\n 和 \r\n 都算作 line end flag
 * @return char size of line
 */
static int syncReadLine(int fd, char *ptr, ssize_t size, int timeout) {
    ssize_t nread = 0;
    size--;
    while (size != 0) {
        char c;
        if (syncRead(fd, &c, 1, timeout) == -1) {
            return -1;
        }
        if (c == '\n') {
            *ptr = '\0';
            if (nread != 0 && *(ptr-1) == '\r') {
                *(ptr-1) = '\0';
            }
            return nread;
        } else {
            *ptr++ = c;
            *ptr = '\0';
            nread++;
        }
    }
    return nread;
}

static int writeFileToClient(int fileFd, int fileLen, int clientFd, int start) {
    char sizebuf[32];
    snprintf(sizebuf, "%d\r\n", fileLen);
    // 1. 写文件大小
    if (syncWrite(clientFd, sizebuf, strlen(sizebuf), 5) == -1) {
        return REDIS_ERR;
    }

    // 2. 写文件内容
    while (fileLen > 0) {
       char buf[1024];
        int nread;
        if (time(NULL) - start > REDIS_MAX_SYNC_TIME) {
            return REDIS_ERR;
        }
        nread = read(fileFd, buf, 1024);
        if (nread == -1) {
            return REDIS_ERR;
        }
        fileLen -= nread;
        if (syncWrite(clientFd, buf, nread, 5) == -1) {
            return REDIS_ERR;
        } 
    }
    return syncWrite(fileFd, "\r\n", 2, 5) == -1 ? REDIS_ERR : REDIS_OK;
}

/**
 * 将 dbfile 的内容同步到 client，如果同步成功 client 就是 slave 了, 否则标记为 close 状态
 */
static void syncCommand(redisClient *c) {
    time_t start = time(NULL);
    int fd = -1;
    redisLog(REDIS_NOTICE, "Slave ask for syncronization");
    if (flushClientOutput(c) == REDIS_ERR || saveDb(server.dbfilename) != REDIS_OK) {
        goto closeconn;
    }

    struct stat sb;
    fd = open(server.dbfilename, O_RDONLY);
    if (fd == -1 || fstat(fd, &sb) == -1) {
        goto closeconn;
    }
    writeFileToClient(fd, sb.st_size, c->fd, start);
    close(fd);

    c->flags |= REDIS_SLAVE;
    c->slaveseldb = 0;
    if (!listAddNodeTail(server.slaves, c)) {
        oom("listAddNodeTail of slave");
    }
    redisLog(REDIS_NOTICE, "Syncronization with slave succeeded");
    return;

    closeconn:
        if (fd != -1) {
            close(fd);
        }
        c->flags |= REDIS_CLOSE;
        redisLog(REDIS_WARNING, "Syncronization with slave failed.");
        return;
}

/**
 * 读取 master 上的 rdb 文件，替换掉自己的 dbfile
 * @param dumpsize master 上 rdb 文件的大小
 * @param fd master 连接
 */
static int dumpMasterRDB(int dumpsize, int fd) {
    char tmpfile[256];
    snprintf(tmpfile, 256, "temp-%d.%ld.rdb", (int)time(NULL), (long int)random());
    int dfd = open(tmpfile, O_CREAT|O_WRONLY, 0644);
    if (dfd == -1) {
        redisLog(REDIS_WARNING, "Opening the temp file needed for MASTER <-> SLAVE syncrhonization: %s", strerror(errno));
        return REDIS_ERR;
    }

    char buf[1024];
    while (dumpsize > 0) {
        int nread = read(fd, buf, (dumpsize < 1024 ? dumpsize : 1024));
        if (nread == -1) {
            redisLog(REDIS_WARNING, "I/O error trying to sync with MASTER: %s", strerror(errno));
            close(dfd);
            return REDIS_ERR;
        }

        int nwritten = write(dfd, buf, nread);
        if (nwritten == -1) {
            redisLog(REDIS_WARNING, "Write error writing to the DB dump file needed for MASTER <-> SLAVE synchronization: %s", strerror(errno));
            close(dfd);
            return REDIS_ERR;
        }

        dumpsize -= nread;
    }
    close(dfd);
    if (rename(tmpfile, server.dbfilename) == -1) {
        redisLog(REDIS_WARNING, "Failed trying to rename the temp DB into dump.rdb in MASTER <-> SLAVE syncrhonization: %s", strerror(errno));
        unlink(tmpfile);
        return REDIS_ERR;
    }

}

static int syncWithMaster() {
    int fd = anetTcpConnect(NULL, server.masterhost, server.masterhost);
    if (fd == -1) {
        redisLog(REDIS_WARNING, "unable to connect to MASTER: %s", strerror(errno));
        return REDIS_ERR;
    }

    // 1. 发送 SYNC 标记，准备同步
    if (syncWrite(fd, "SYNC \r\n", 7, 5) == -1) {
        close(fd);
        redisLog(REDIS_WARNING, "I/O error writing to MASTER: %s", strerror(errno));
        return REDIS_ERR;
    }

    // 2. 读取 master 的 rdb
    char buf[1024];
    if (syncRead(fd, buf, 1024, 5) == -1) {
        close(fd);
        redisLog(REDIS_WARNING, "I/O error reading bulk count from MASTER: %s", strerror(errno));
        return REDIS_ERR;
    }
    int dumpsize = atoi(buf);
    redisLog(REDIS_NOTICE, "Receiving %d bytes data dump from MASTER", dumpsize);
    if (dumpMasterRDB(dumpsize, fd) != REDIS_OK) {
        close(fd);
        return REDIS_ERR;
    }
    
    emptyDb();
    if (loadDb(server.dbfilename) != REDIS_OK) {
        redisLog(REDIS_WARNING, "Failed tring to load the MASTER synchronization DB from disk");
        close(fd);
        return REDIS_ERR;
    }

    server.master = createClient(fd);
    server.master->flags |= REDIS_MASTER;
    server.replstate = REDIS_REPL_CONNECTED;
    return REDIS_OK;
}

/* =========================== RDB save ===================== */

/**
 * 格式: sds length, sds content
 */
static int writeSdsToFile(sds sval, FILE *fp) {
    size_t valsize = sdslen(sval);
    uint32_t len = htonl(valsize);
    if (fwrite(&len, 4, 1, fp) == 0 || (valsize > 0 && fwrite(sval, valsize, 1, fp) == 0)) {
        return REDIS_ERR;
    } else {
        return REDIS_OK;
    }
}

/**
 * 格式: list size, [entry length, entry content, ...]
 */
static int writeListToFile(list *list, FILE *fp) {
    listNode *node = list->head;
    uint32_t len = htonl(listLength(list));
    if (fwrite(&len, 4, 1, fp) == 0) {
        return REDIS_ERR;
    }
    while (node != NULL) {
        robj *eleobj = listNodeValue(node);
        size_t valsize = sdslen(eleobj->ptr);
        len = htonl(valsize);
        if (fwrite(&len, 4, 1, fp) == 0 || (valsize > 0 && fwrite(eleobj->ptr, valsize, 1, fp) == 0)) {
            return REDIS_ERR;
        }
        node = node->next;
    }
    return REDIS_OK;
}

/**
 * 格式: set size, [entry length, entry content, ...]
 */
static int writeSetToFile(dict *set, FILE *fp) {
    dictIterator *it = dictGetIterator(set);
    if (it == NULL) {
        oom("dictGetIterator");
    }
    uint32_t len = htonl(dictGetHashTableUsed(set));
    if (fwrite(&len, 4, 1, fp) == 0) {
        return REDIS_ERR;
    }
    dictEntry *entry;
    while ((entry = dictNext(it)) != NULL) {
        robj *eleobj = dictGetEntryKey(entry);
        size_t keylen = sdslen(eleobj->ptr);
        len = htonl(keylen);
        if (fwrite(&len, 4, 1, fp) == 0 || (keylen > 0 && fwrite(eleobj->ptr, keylen, 1, fp) == 0)) {
            return REDIS_ERR;
        }
    }
    dictReleaseIterator(it);
    return REDIS_OK;
}

/**
 * @param it 一个 redis db 对应一个 dict, 这里的it就是该 dict 的 iterator
 * 格式: [type, key length, key content, value], vaulue 可能是: sds, list, set; key 一定是 sds
 */
static int writeOneDBToFile(dictIterator *it, FILE *fp) {
    dictEntry *entry;
    while ((entry = dictNext(it)) != NULL) {
        robj *key = dictGetEntryKey(entry);
        robj *o = dictGetEntryVal(entry);
        uint8_t type = o->type; 
        if (fwrite(&type, 1, 1, fp) == 0) {
            return REDIS_ERR;
        }

        uint32_t len = htonl(sdslen(key->ptr));
        if (fwrite(&len, 4, 1, fp) == 0 || fwrite(key->ptr, sdslen(key->ptr), 1, fp) == 0) {
            return REDIS_ERR;
        }

        int status = REDIS_ERR;
        switch (type) {
        case REDIS_STRING:
            status = writeSdsToFile(o->ptr, fp); 
            break;
        case REDIS_LIST:
            status = writeListToFile(o->ptr, fp);
            break;
        case REDIS_SET:
            status = writeSetToFile(o->ptr, fp);
            break;
        default:
            assert(false);
        }
        if (status != REDIS_OK) {
            return REDIS_ERR;
        }
    }

}

/**
 * 先写 temp file, 写成功后再原子的 rename 成 filename
 * 格式：
 *     REDIS0000
 *     [254, db_no, db content, ...] // 1. 254 REDIS_SELECTDB; 2. 无数据的DB忽略掉
 *     255 // REDIS_EOF
 *      
 * 
 */
static int saveDb(char *filename) {
    char tmpfile[256];
    snprintf(tmpfile, 256, "temp-%d.%ld.rdb", (int) time(NULL), (long int) random());
    FILE *fp = fopen(tmpfile, "w");
    if (fp == NULL) {
        redisLog(REDIS_WARNING, "Failed saving the DB: %s", strerror(errno));
        return REDIS_ERR;
    }

    /**
     * fwrite(ptr, size, nsize, stream)
     * ptr: 要写入的内容
     * size: 每个元素的大小，单位是字节
     * nsize: 写入的元素个数
     */
    if (fwrite("REDIS0000", 9, 1, fp) == 0) {
        goto werr;
    }

    dictIterator *dictIt = NULL;
    for (int i = 0; i < server.dbnum; i++) {
        dict *d = server.dict[i];
        if (dictGetHashTableUsed(d) == 0) {
            continue;
        }

        uint8_t type = REDIS_SELECTDB;
        uint32_t len = htonl(i);
        if (fwrite(&type, 1, 1, fp) == 0) { goto werr; }
        if (fwrite(&len, 4, 1, fp) == 0) { goto werr; }

        dictIt = dictGetIterator(d);
        if (dictIt == NULL) {
            fclose(fp);
            return REDIS_ERR;
        }
        int status = writeOneDBToFile(dictIt, fp);
        if (status != REDIS_OK) {
            goto werr;
        }
        dictReleaseIterator(dictIt);
    }
    uint8_t type = REDIS_EOF;
    if (fwrite(&type, 1, 1, fp) == 0) { goto werr; }
    fflush(fp);
    // flush file data and metadata
    fsync(fileno(fp));
    fclose(fp);

    // 如果 DB file 生成成功了，原子的切换
    if (rename(tmpfile, filename) == -1) {
        redisLog(REDIS_WARNING, "Error moving temp DB file on the final destionation: %s", strerror(errno));
        unlink(tmpfile);
        return REDIS_ERR;
    } else {
        redisLog(REDIS_NOTICE, "DB saved on disk");
        server.dirty = 0;
        server.lastsave = time(NULL);
        return REDIS_OK;
    }

    werr:
        flose(fp);
        unlink(tmpfile);
        redisLog(REDIS_WARNING, "Write error saving DB on disk: %s", strerror(errno));
        if (dictIt != NULL) {
            dictReleaseIterator(dictIt);
        }
        return REDIS_ERR;
}

/**
 * @return REDIS_OK on success, or REDIS_ERR
 */
static int saveDbBackground(char *filename) {
    if (server.bgsaveinprogress) {
        return REDIS_ERR;
    }

    pid_t childpid = fork();
    if (childpid == 0) {
        // todo: why close server.fd ?
        close(server.fd);
        if (saveDb(filename) == REDIS_OK) {
            exit(0);
        } else {
            exit(1);
        }
    } else {
        redisLog(REDIS_NOTICE, "Background saving started by pid %d", childpid);
        server.bgsaveinprogress = true;
        return REDIS_OK;
    }
    return REDIS_OK;
}

static void waitBgsaveFinish() {
    int statloc;
    if (wait4(-1, &statloc, WNOHANG, NULL)) {
        int exitcode = WEXITSTATUS(statloc);
        if (exitcode == 0) {
            redisLog(REDIS_NOTICE, "Background saving terminated with success");
            server.dirty = 0;
            server.lastsave = time(NULL);
        } else {
            redisLog(REDIS_WARNING, "Background saving error");
        }
        server.bgsaveinprogress = false;
    }
}

static void bgsaveIfNeed() {
    time_t now = time(NULL);
    for (int i = 0; i < server.saveparamslen; i++) {
        struct saveparam *sp = server.saveparams + i;
        if (server.dirty >= sp->changes && (now - server.lastsave) > sp->seconds) {
            redisLog(REDIS_NOTICE, "%d changes in %d seconds, Saving...", sp->changes, sp->seconds);
            saveDbBackground(server.dbfilename);
            break;
        }
    }
}

/**
 * 从文件中读取出一个 string object，格式: lengh, content
 * @param preallocateLoadBuf 大小为REDIS_LOADBUF_LEN，如果要读取的 length 小于这个值可以不用再申请内存了
 * @return string object if success, otherwise NULL
 */
static robj *deserializeStringObject(int fp, char *preallocLoadBuf) {
    uint32_t len;
    if (fread(&len, 4, 1, fp) == 0) {
        return NULL;
    }
    len = ntohl(len);

    char *buf = preallocLoadBuf;
    if (len > REDIS_LOADBUF_LEN) {
        buf = zmalloc(len);
        if (buf == NULL) {
            oom("Loading DB from file.");
        }
    }

    if (len > 0 && fread(buf, len, 1, fp) == 0) {
        if (buf != preallocLoadBuf) {
            zfree(buf);
        }
        return NULL;
    }

    robj *o = createStringObject(buf, len);
    if (buf != preallocLoadBuf) {
        zfree(buf);
    }
    return o;
}

static robj *deserializeList(int fp, char *preallocateLoadBuf) {
    uint32_t listlen;
    if (fread(&listlen, 4, 1, fp) == 0) {
        return NULL;
    }
    listlen = ntohl(listlen);
    robj *o = createListObject();
    while (listlen-- > 0) {
        robj *ele = deserializeStringObject(fp, preallocateLoadBuf);
        if (ele == NULL) {
            // todo: 不需要 free o
            return NULL;
        }
        if (listAddNodeTail((list *)o->ptr, ele) == NULL) {
            oom("listAddNodeTail");
        }
    }
    return o;
}

static robj *deserializeSet(int fp, char *preallocateLoadBuf) {
    uint32_t setlen;
    if (fread(&setlen, 4, 1, fp) == 0) {
        return NULL;
    }
    setlen = ntohl(setlen);
    robj *set = createSetObject();
    while (setlen-- > 0) {
        robj *ele = deserializeStringObject(fp, preallocateLoadBuf);
        if (ele == NULL) {
            return NULL;
        }
        if (dictAdd((dict *) set->ptr, ele, NULL) == NULL) {
            oom("dictAdd");
        }
    }
    return set;
}

/**
 * @return REDIS_ERR if any error occur
 *         REDIS_SELECTDB this db is finished
 *         REDIS_EOF      the dbfile is reach end
 */
static int loadOneDbFromFile(int fp) {
    uint32_t dbid;
    if (fread(&dbid, 4, 1, fp) == 0) {
        return REDIS_ERR;
    }
    dbid = ntohl(dbid);
    if (dbid >= (unsigned) server.dbnum) {
        redisLog(REDIS_WARNING,"FATAL: Data file was created with a Redis server compiled to handle more than %d databases. Exiting\n", server.dbnum);
        exit(1);
    }
    dict *d = server.dict[dbid];

    // 预先分配的内存，如果要读取的数据可以放下则可以减少内存申请
    char buf[REDIS_LOADBUF_LEN]; 

    while (true) {
        uint8_t type;
        if (fread(&type, 1, 1, fp) == 0) {
            return REDIS_ERR;
        }
        // 当前db结束了
        if (type == REDIS_SELECTDB || type == REDIS_EOF) {
            return type;
        }

        robj *key = deserializeStringObject(fp, buf);
        if (key == NULL) {
            return REDIS_ERR;
        }

        robj *value = NULL;
        switch (type) {
            case REDIS_STRING:
                value = deserializeStringObject(fp, buf);
                break;
            case REDIS_LIST:
                value = deserializeList(fp, buf);
                break;
            case REDIS_SET:
                value = deserializeSet(fp, buf);
                break;
            default:
                assert(false);
        }
        if (value == NULL) {
            return REDIS_ERR;
        }
        if (dictAdd(d, key, value) == DICT_ERR) {
            redisLog(REDIS_WARNING, "Loading DB, duplicated key found! Unrecoverable error, exiting now.");
            exit(1);
        }
    }
    // actuall unreach
    return REDIS_OK;
}

/**
 * 读 rdb 文件，重构所有db
 * REDIS0000
 * [254, db_no, db content, ...] // 1. 254 REDIS_SELECTDB; 2. 无数据的DB忽略掉
 *    db content: [type, key length, key content, value, ...] value需要根据type来解析
 * 255 // REDIS_EOF
 * @return REDIS_OK if success, otherwise REDIS_ERR
 */
static int loadDb(char *filename) {
    char *fp = fopen(filename, "r");
    if (fp == NULL) {
        return REDIS_ERR;
    }
    char buf[9];
    if (fread(buf, 9, 1, fp) == 0) {
        goto eoferr;
    }
    if (memcmp(buf, "REDIS0000", 9) != 0) {
        fclose(fp);
        redisLog(REDIS_WARNING, "Wrong signature trying to load DB from file");
        return REDIS_ERR;
    }

    uint8_t type;
    if (fread(&type, 1, 1, fp) == 0) {
        goto eoferr;
    }
    while (type == REDIS_SELECTDB) {
        type = loadOneDbFromFile(fp);
    }
    if (type != REDIS_EOF) {
        goto eoferr;
    }

    fclose(fp);
    return REDIS_OK;

    eoferr: 
        fclose(fp);
        redisLog(REDIS_WARNING, "Short read loading DB, Unrecoverable error, exiting now.");
        exit(1);
        return REDIS_ERR; // avoid compile warning
}

/* ========================= Comands ========================== */
static void pingCommand(redisClient *c) {
    addReply(c, shared.pong);
}

static void echoCommand(redisClient *c) {
    addReplySds(c, sdscatprintf(sdsempty(), "%d\r\n", (int)sdslen(c->argv[1]->ptr)));
    addReply(c, c->argv[1]);
    addReply(c, shared.crlf);
}

/* ===================== Strings ======================== */
/**
 * c->argv: 0: set; 1: key; 2: value
 * @param nx is not exist, if true only key not exist will add
 */
static void setGenericCommand(redisClient *c, bool nx) {
    int retval = dictAdd(c->dict, c->argv[1], c->argv[2]);
    if (retval == DICT_ERR) {
        if (nx) {
            // key 存在，不做任何操作
            addReply(c, shared.zero);
            return;
        } else {
            // todo: 是不是应该把这个判断提前，如果不是nx，直接使用replace
            dictReplace(c->dict, c->argv[1], c->argv[2]);
            incrRefCount(c->argv[2]);
        }
    } else {
        incrRefCount(c->argv[1]);
        incrRefCount(c->argv[2]);
    }
    server.dirty++;
    addReply(c, nx ? shared.one : shared.ok);
}

static void setCommand(redisClient *c) {
    return setGenericCommand(c, false);
}

static void setnxCommand(redisClient *c) {
    return setGenericCommand(c, true);
}

static void getCommand(redisClient *c) {
    dictEntry *de = dictFind(c->dict, c->argv[1]);
    if (de != NULL) {
        robj *o = dictGetEntryVal(de);
        if (o->type != REDIS_STRING) {
            addReply(c, shared.wrongtypeerrbulk);
        } else {
            addReplySds(c, sdscatprintf(sdsempty(), "%d\r\n", (int) sdslen(o->ptr)));
            addReply(c, o);
            addReply(c, shared.crlf);
        }
    } else {
        addReply(c, shared.nil);
    }
}

static long long dictEntryStringValueToLL(dictEntry *de) {
    if (de == NULL) {
        return 0;
    }
    robj *o = dictGetEntryVal(de);
    if (o->type != REDIS_STRING) {
        return 0;
    } else {
        char *eptr;
        return strtoll(o->ptr, &eptr, 10);
    }
}
/**
 * @param delta 加/减数，负数表示减
 */
static void incrDecrCommand(redisClient *c, int delta) {
    dictEntry *de = dictFind(c->dict, c->argv[1]);
    long long value = dictEntryStringValueToLL(de);

    value += delta;

    robj *o = createObject(REDIS_STRING, sdscatprintf(sdsempty(), "%lld", value));
    int retval = dictAdd(c->dict, c->argv[1], o);
    if (retval != DICT_ERR) {
        incrRefCount(c->argv[1]);
    } else {
        dictReplace(c->dict, c->argv[1], o);
    }
    server.dirty++;
    addReply(c, o);
    addReply(c, shared.crlf);
}

static void incrCommand(redisClient *c) {
    return incrDecrCommand(c, 1);
}

static void incrbyCommand(redisClient *c) {
    int incr = atoi(c->argv[2]->ptr);
    return incrDecrCommand(c, incr);
}

static void decrCommand(redisClient *c) {
    return incrDecrCommand(c, -1);
}

static void decrbyCommand(redisClient *c) {
    int incr = atoi(c->argv[2]->ptr);
    return incrDecrCommand(c, -incr);
}

/* ==================== Type agnostic commands ====================== */
static void delCommand(redisClient *c) {
    if (dictDelete(c->dict, c->argv[1]) == DICT_OK) {
        server.dirty++;
        addReply(c, shared.one);
    } else {
        addReply(c, shared.zero);
    }
}

static void existCommand(redisClient *c) {
    dictEntry *de = dictFind(c, c->argv[1]);
    addReply(c, de != NULL ? shared.one : shared.zero);
}

static void selectCommand(redisClient *c) {
    int id = atoi(c->argv[1]->ptr);
    if (selectDb(c, id) != REDIS_ERR) {
        addReply(c, shared.ok);
    } else {
        addReplySds(c, "-ERR invalid DB index\r\n");
    }
}

static void randomkeyCommand(redisClient *c) {
    dictEntry *de = dictGetRandomKey(c->dict);
    if (de != NULL) {
        addReply(c, dictGetEntryKey(de));
        addReply(c, shared.crlf);
    } else {
        addReply(c, shared.crlf);
    }
}

static void keysCommand(redisClient *c) {
    sds pattern = c->argv[1]->ptr;
    int plen = sdslen(pattern);

    dictIterator *it = dictGetIterator(c->dict);
    if (it == NULL) {
        oom("dictGetIterator");
    }

    robj *lenobj = createObject(REDIS_STRING, NULL);
    addReply(c, lenobj);
    decrRefCount(lenobj);

    int keyslen = 0;
    int numkeys = 0;
    dictEntry *de;
    while ((de = dictNext(it)) != NULL) {
        robj *keyobj = dictGetEntryKey(de);
        sds key = keyobj->ptr;
        if ((pattern[0] == '*' && pattern[1] == '\0') || stringmatchlen(pattern, plen, key, sdslen(key), 0)) {
            if (numkeys != 0) {
                addReply(c, shared.space);
            }
            addReply(c, keyobj);
            numkeys++;
            keyslen += sdslen(key);
        }
    }
    dictReleaseIterator(it);
    // numkeys-1 表示空格的数量?
    // todo: 这个值什么时候发送呢
    lenobj->ptr = sdscatprintf(sdsempty(), "%lu\r\n", keyslen + (numkeys ? numkeys-1 : 0));
    addReply(c, shared.crlf);
}

static void dbsizeCommand(redisClient *c) {
    sds size = sdscatprintf(sdsempty(), "%lu\r\n", dictGetHashTableUsed(c->dict));
    addReply(c, size);
}

static void lastsaveCommand(redisClient *c) {
    addReply(c, sdscatprintf(sdsempty(), "%lu\r\n", server.lastsave));
}

static void typeCommand(redisClient *c) {
    dictEntry *entry = dictFind(c->dict, c->argv[1]);
    char *type;
    if (entry != NULL) {
        robj *o = dictGetEntryVal(entry);
        if (o->type == REDIS_STRING) {
            type = "string";
        } else if (o->type == REDIS_LIST) {
            type = "list";
        } else if (o->type == REDIS_SET) {
            type = "set";
        } else {
            type = "unknow";
        }
    } else {
        type = "none";
    }
    addReplySds(c, sdsnew(type));
    addReply(c, shared.crlf);
}

static void saveCommand(redisClient *c) {
    if (saveDb(server.dbfilename) == REDIS_OK) {
        addReply(c, shared.ok);
    } else {
        addReply(c, shared.err);
    }
}

static void bgsaveCommand(redisClient *c) {
    if (server.bgsaveinprogress) {
        addReplySds(c, sdsnew("-ERR background save already in progress\r\n"));
        return;
    }
    if (saveDbBackground(server.dbfilename) == REDIS_OK) {
        addReply(c, shared.ok);
    } else {
        addReply(c, shared.err);
    }
}

static void shutdownCommand(redisClient *c) {
    redisLog(REDIS_WARNING, "User requested shutdown, saving DB...");
    if (saveDb(server.dbfilename) == REDIS_OK) {
        redisLog(REDIS_WARNING, "Server exit now, bye bye...");
        exit(1);
    } else {
        redisLog(REDIS_WARNING, "Error trying to save the db, can't exit");
        addReplySds(c, sdsnew("-RR can't quit, problems saving the DB\r\n"));
    }
}

/**
 * @param nx is not exist. if nx is true and the new key is exist, will not rename
 */
static void renameGenericCommand(redisClient *c, bool nx) {
    if (sdscmp(c->argv[1]->ptr, c->argv[2]->ptr) == 0) {
        if (nx) {
            addReply(c, shared.minus3);
        } else {
            addReplySds(c, sdsnew("-ERR src and dest are the same\r\n"));
        }
    }

    dictEntry *de = dictFind(c->dict, c->argv[1]);
    if (de == NULL) {
        addReply(c, nx ? shared.minus1 : shared.nokeyerr);
        return;
    }

    robj *o = dictGetEntryVal(de);
    // todo: 这里为什么要增加引用次数?
    incrRefCount(o);
    if (dictAdd(c->dict, c->argv[2], o) == DICT_ERR) {
        // the new key is exist, rename failed
        if (nx) {
            decrRefCount(o);
            addReply(c, shared.zero);
            return;
        } else {
            dictReplace(c->dict, c->argv[2], o);
        }
    } else {
        incrRefCount(c->argv[2]);
    }
    dictDelete(c->dict, c->argv[1]);
    server.dirty++;
    addReply(c, nx ? shared.one : shared.ok);
}

static void renameCommand(redisClient *c) {
    renameGenericCommand(c, false);
}

static void renamenxCommand(redisClient *c) {
    renameGenericCommand(c, true);
}

static void moveCommand(redisClient *c) {
    dict *src = c->dict;
    int srcid = c->dictid;

    if (selectDb(c, atoi(c->argv[2]->ptr)) == REDIS_ERR) {
        addReply(c, shared.minus4);
        return;
    }
    dict *dst = c->dict;
    c->dict = src;
    c->dictid = srcid;
    if (src == dst) {
        addReply(c, shared.minus3);
        return;
    }

    dictEntry *de = dictFind(c->dict, c->argv[1]);
    if (de == NULL) {
        addReply(c, shared.zero);
        return;
    }
    robj *key = dictGetEntryKey(de);
    robj *o = dictGetEntryVal(de);
    if (dictAdd(dst, key, o) == DICT_ERR) {
        addReply(c, shared.zero);
        return;
    }
    incrRefCount(key);
    incrRefCount(o);

    // move 成功，从原来的 db 中删除
    dictDelete(src, c->argv[1]);
    server.dirty++;
    addReply(c, shared.one);
}



static void freeClientArgv(redisClient *c) {
    for (int i = 0; i < c->argc; i++) {
        decrRefCount(c->argv[i]);
    }
    c->argc = 0;
}

/**
 * 1. 从 file event 中删除 redisClient->fd
 * 2. 释放 redisClient->querybuf
 * 3. 释放 redisClient->argv
 * 4. 从 server.clients 中删除 c
 * 5. if c is slave, delete from server.slaves
 * 6. if c is master, delete server.master and set server.replstate to REDIS_REPL_CONNECT
 */
static void freeClient(redisClient *c) {
    // todo: 同一个 client 会作为两个 file event 吗
    aeDeleteFileEvent(server.el, c->fd, AE_READABLE);
    aeDeleteFileEvent(server.el, c->fd, AE_WRITABLE);
    sdsfree(c->querybuf);
    freeClientArgv(c);
    close(c->fd);

    listNode *node = listSearchKey(server.clients, c);
    assert(node != NULL);
    listDelNode(server.clients, node);
    if (isSlave(c->flags)) {
        node = listSearchKey(server.slaves, c);
        assert(node != NULL);
        listDelNode(server.slaves, node);
    }
    if (isMaster(c->flags)) {
        server.master = NULL;
        server.replstate = REDIS_REPL_CONNECT;
    }
    zfree(c);
}

/**
 * 
 * @return 1 client is still alive and valid, other operations can be performed by the caller. 
 *         0 client was destroied
 */
static int processCommand(redisClient *c) {
    sdstolower(c->argv[0]->ptr);
    if (strcmp(c->argv[0]->ptr, "quit") == 0) {
        freeClient(c);
        return 0;
    }
    
    struct redisCommand *cmd = lookupCommand(c->argv[0]->ptr);
    if (c == NULL) {
        addReplySds(c, sdsnew("-RR unknow command\r\n"));
        resetClient(c);
        return 1;
    } else if ((cmd->arity > 0 && cmd->arity != c->argc) || (c->argc < -cmd->arity)) {
        addReply(c, sdsnew("-ERR wrong number of arguments\r\n"));
    }

}

static void readQueryFromClient(aeEventLoop *el, int fd, void *privdata, int mask) {
    REDIS_NOTUSED(el); REDIS_NOTUSED(mask);

    redisClient *c = (redisClient *) privdata;
    char buf[REDIS_QUERYBUF_LEN];
    int nread = read(fd, buf, REDIS_QUERYBUF_LEN);
    if (nread == -1) {
        if (errno == EAGAIN) {
            nread = 0;
        } else {
            redisLog(REDIS_DEBUG, "Reading from client: %s", strerror(errno));
            freeClient(c);
            return;
        }
    } else if (nread == 0) {
        redisLog(REDIS_DEBUG, "Client closed connection");
        freeClient(c);
        return;
    }
    if (nread > 0) {
        c->querybuf = sdscatlen(c->querybuf, buf, nread);
        c->lastinteraction = time(NULL);
    } else {
        return;
    }

    again:
        if (c->bulklen == -1) {

        } else {
            int qbl = sdslen(c->querybuf);
            if (c->bulklen <= qbl) {
                // argv 是提前创建好的;                                去掉\r\n
                c->argv[c->argc] = createStringObject(c->querybuf, c->bulklen-2);
                c->argc++;
                c->querybuf = sdsrange(c->querybuf, c->bulklen, -1);
                processCommand(c);
                return;
            }
        }
}

static int selectDb(redisClient *c, int id) {
    if (id < 0 || id >= server.dbnum) {
        return REDIS_ERR;
    }
    c->dict = server.dict[id];
    c->dictid = id;
    return REDIS_OK;
}

/**
 * 创建 client, 并且会注册 readQueryFromClient 的 file event
 */
static redisClient *createClient(int fd) {
    redisClient *c = zmalloc(sizeof(*c));
    if (c == NULL) {
        return NULL;
    }
    anetNonBlock(NULL, fd);
    anetTcpNoDelay(NULL, fd);
    selectDb(c, 0);
    c->fd = fd;
    c->querybuf = sdsempty();
    c->argc = 0;
    c->bulklen = -1;
    c->sentlen = 0;
    c->flags = 0;
    c->lastinteraction = time(NULL);
    if ((c->reply = listCreate()) == NULL) {
        oom("listCreate");
    }
    listSetFreeMethod(c->reply, decrRefCount);
    if (aeCreateFileEvent(server.el, c->fd, AE_READABLE, readQueryFromClient, c, NULL) == AE_ERR) {
        freeClient(c);
        return NULL;
    }
    if (!listAddNodeTail(server.clients, c)) {
        oom("listAddNodeTail");
    }
    return c;
}

static void addReply(redisClient *c, robj *obj) {
    if (listLength(c->reply) == 0 && aeCreateFileEvent(server.el, c->fd, AE_WRITABLE, sendReplyToClient, c, NULL) == AE_ERR) {
        return;
    }

    if (listAddNodeTail(c->reply, obj) == NULL) {
        oom("listAddNodeTail");
    }
    incrRefCount(obj);
}

static void addReplySds(redisClient *c, sds s) {
    robj *o = createObject(REDIS_STRING, s);
    addReply(c, o);
    decrRefCount(o);
}


static void closeTimeoutClients() {
    listIter *it = listGetIterator(server.clients, AL_START_HEAD);
    if (it == NULL) {
        return;
    }

    time_t now = time(NULL);
    listNode *node;
    while ((node = listNextElement(it)) != NULL) {
        redisClient *c = listNodeValue(node);
        if (isSlave(c->flags) && (now - c->lastinteraction > server.maxidletime)) {
            redisLog(REDIS_DEBUG, "Closing idle client: %d-%d", c->dictid, c->fd);
            freeClient(c);
        }
    }
    listReleaseIterator(it);
}


static void redisDbResize(int loops) {
    for (int i = 0; i < server.dbnum; i++) {
        int size = dictGetHashTableSize(server.dict[i]);
        int used = dictGetHashTableUsed(server.dict[i]);
        if ((loops%5) == 0 && used > 0) {
            redisLog(REDIS_DEBUG, "DB %d: %d keys in %d slots HT.", i, used, size);
        }
        if (size > 0 && used > 0 && size > REDIS_HT_MINSLOTS && (used * 100 / size < REDIS_HT_MINFILL)) {
            redisLog(REDIS_NOTICE, "The hash table %d is too sparse, resize it...");
            dictResize(server.dict[i]);
            redisLog(REDIS_NOTICE, "Hash table %d resized.", i);
        }
    }
}

/**
 * 1. 更新 usedmemory
 * 2. resize db if needed
 * 3. log clients number info
 * 4. close timeout clients
 * 5. background save db if needed
 * 6. sync with master if needed
 */
int serverCron(struct aeEventLoop *eventLoop, long long id, void *clientData) {
    REDIS_NOTUSED(eventLoop);
    REDIS_NOTUSED(id);
    REDIS_NOTUSED(clientData);

    // update the global state with the amount of used memory
    server.usedmemory = zmalloc_used_memory();

    int loops = server.cronloops++;
    redisDbResize(loops);

    // 打印连接的 clients 数
    if (loops%5 == 0) {
        redisLog(REDIS_DEBUG, "%d clients connected (%d slaves), %d bytes in use",
        listLength(server.clients) - listLength(server.slaves), listLength(server.slaves), server.usedmemory);
    }

    if (loops%10 == 0) {
        closeTimeoutClients();
    }

    if (server.bgsaveinprogress) {
        waitBgsaveFinish();
    } else {
        bgsaveIfNeed();
    }

    if (server.replstate == REDIS_REPL_CONNECT) {
        redisLog(REDIS_NOTICE, "Connecting to MASTER...");
        if (syncWithMaster() == REDIS_OK) {
            redisLog(REDIS_NOTICE, "MASTER <-> SLAVE sync succeeded");
        }
    }

    return 1000;
}

/**
 * 在 seconds 内有 changes 修改就执行保存操作
 */
static void appendServerSaveParams(time_t seconds, int changes) {
    server.saveparams = zrealloc(server.saveparams, sizeof(struct saveparam) * (server.saveparamslen + 1));
    if (server.saveparams == NULL) {
        oom("appendServerSaveParams");
    }
    server.saveparams[server.saveparamslen].seconds = seconds;
    server.saveparams[server.saveparamslen].changes = changes;
    server.saveparamslen++;
}

static void ResetServerSaveParams() {
    zfree(server.saveparams);
    server.saveparams = NULL;
    server.saveparamslen = 0;
}

/**
 * 初始化 server 中的配置部分
 */
static void initServerConfig() {
    server.dbnum = REDIS_DEFULT_DBNUM;
    server.port = REDIS_SERVERPORT;
    server.verbosity = REDIS_DEBUG;
    server.maxidletime = REDIS_MAXIDLETIME;
    server.logfile = NULL; // means log on standard output
    server.bindaddr = NULL;
    server.glueoutputbuf = 1;
    server.daemonize = false;
    server.dbfilename = "dump.rdb";

    server.saveparams = NULL;
    ResetServerSaveParams();
    appendServerSaveParams(60 * 60, 1);
    appendServerSaveParams(60 * 5, 100);
    appendServerSaveParams(60 * 1, 10000);

    /** Replication related */
    server.isslave = false;
    server.masterhost = NULL;
    server.masterport = REDIS_SERVERPORT;
    server.master = NULL;
    server.replstate = REDIS_REPL_NONE;
}

/**
 * 初始化 server 中的重要部分:
 * 1. 创建各种数据结构
 * 2. 创建每个 db 对象(dict)
 * 3. 启动 server 的 tcp 监听
 * 4. 启动定时任务：1) resize db; 2) close timeout clients; 3) bgsave; 4) sync master
 */
static void initServer() {
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    server.clients = listCreate();
    server.slaves = listCreate();
    server.objfreelist = listCreate();
    createSharedObjects();
    server.el = aeCreateEventLoop();
    server.dict = zmalloc(sizeof(dict *) * server.dbnum);
    if (server.dict == NULL || server.clients == NULL || server.slaves == NULL || server.el == NULL || server.objfreelist == NULL) {
        oom("server initialization");
    }
    for (int i = 0; i < server.dbnum; i++) {
        server.dict[i] = dictCreate(&hashDictType, NULL);
        if (server.dict[i] == NULL) {
            oom("dictCreate");
        }
    }

    server.fd = anetTcpServer(server.neterr, server.port, server.bindaddr);
    if (server.fd == ANET_ERR) {
        redisLog(REDIS_WARNING, "Opening Tcp port: %s", server.neterr);
        exit(1);
    }

    server.cronloops = 0;
    server.bgsaveinprogress = 0;
    server.lastsave = time(NULL);
    server.dirty = 0;
    server.usedmemory = 0;
    server.stat_numcommands = 0;
    server.stat_numconnections = 0;
    server.stat_starttime = time(NULL);
    aeCreateTimeEvent(server.el, 1000, serverCron, NULL, NULL);
}

static void emptyDb() {
    for (int i = 0; i < server.dbnum; i++) {
        dictEmpty(server.dict[i]);
    }
}

/* I agree, this is a very rudimental way to load a configuration...
   will improve later if the config gets more complex */
static void loadServerConfig(char *filename) {
    FILE *fp = fopen(filename,"r");
    char buf[REDIS_CONFIGLINE_MAX+1], *err = NULL;
    int linenum = 0;
    sds line = NULL;
    
    if (!fp) {
        redisLog(REDIS_WARNING,"Fatal error, can't open config file");
        exit(1);
    }
    while(fgets(buf,REDIS_CONFIGLINE_MAX+1,fp) != NULL) {
        sds *argv;
        int argc, j;

        linenum++;
        line = sdsnew(buf);
        line = sdstrim(line," \t\r\n");

        /* Skip comments and blank lines*/
        if (line[0] == '#' || line[0] == '\0') {
            sdsfree(line);
            continue;
        }

        /* Split into arguments */
        argv = sdssplitlen(line,sdslen(line)," ",1,&argc);
        sdstolower(argv[0]);

        /* Execute config directives */
        if (!strcmp(argv[0],"timeout") && argc == 2) {
            server.maxidletime = atoi(argv[1]);
            if (server.maxidletime < 1) {
                err = "Invalid timeout value"; goto loaderr;
            }
        } else if (!strcmp(argv[0],"port") && argc == 2) {
            server.port = atoi(argv[1]);
            if (server.port < 1 || server.port > 65535) {
                err = "Invalid port"; goto loaderr;
            }
        } else if (!strcmp(argv[0],"bind") && argc == 2) {
            server.bindaddr = zstrdup(argv[1]);
        } else if (!strcmp(argv[0],"save") && argc == 3) {
            int seconds = atoi(argv[1]);
            int changes = atoi(argv[2]);
            if (seconds < 1 || changes < 0) {
                err = "Invalid save parameters"; goto loaderr;
            }
            appendServerSaveParams(seconds,changes);
        } else if (!strcmp(argv[0],"dir") && argc == 2) {
            if (chdir(argv[1]) == -1) {
                redisLog(REDIS_WARNING,"Can't chdir to '%s': %s",
                    argv[1], strerror(errno));
                exit(1);
            }
        } else if (!strcmp(argv[0],"loglevel") && argc == 2) {
            if (!strcmp(argv[1],"debug")) server.verbosity = REDIS_DEBUG;
            else if (!strcmp(argv[1],"notice")) server.verbosity = REDIS_NOTICE;
            else if (!strcmp(argv[1],"warning")) server.verbosity = REDIS_WARNING;
            else {
                err = "Invalid log level. Must be one of debug, notice, warning";
                goto loaderr;
            }
        } else if (!strcmp(argv[0],"logfile") && argc == 2) {
            FILE *fp;

            server.logfile = zstrdup(argv[1]);
            if (!strcmp(server.logfile,"stdout")) {
                zfree(server.logfile);
                server.logfile = NULL;
            }
            if (server.logfile) {
                /* Test if we are able to open the file. The server will not
                 * be able to abort just for this problem later... */
                fp = fopen(server.logfile,"a");
                if (fp == NULL) {
                    err = sdscatprintf(sdsempty(),
                        "Can't open the log file: %s", strerror(errno));
                    goto loaderr;
                }
                fclose(fp);
            }
        } else if (!strcmp(argv[0],"databases") && argc == 2) {
            server.dbnum = atoi(argv[1]);
            if (server.dbnum < 1) {
                err = "Invalid number of databases"; goto loaderr;
            }
        } else if (!strcmp(argv[0],"slaveof") && argc == 3) {
            server.masterhost = sdsnew(argv[1]);
            server.masterport = atoi(argv[2]);
            server.replstate = REDIS_REPL_CONNECT;
        } else if (!strcmp(argv[0],"glueoutputbuf") && argc == 2) {
            sdstolower(argv[1]);
            if (!strcmp(argv[1],"yes")) server.glueoutputbuf = 1;
            else if (!strcmp(argv[1],"no")) server.glueoutputbuf = 0;
            else {
                err = "argument must be 'yes' or 'no'"; goto loaderr;
            }
        } else if (!strcmp(argv[0],"daemonize") && argc == 2) {
            sdstolower(argv[1]);
            if (!strcmp(argv[1],"yes")) server.daemonize = 1;
            else if (!strcmp(argv[1],"no")) server.daemonize = 0;
            else {
                err = "argument must be 'yes' or 'no'"; goto loaderr;
            }
        } else {
            err = "Bad directive or wrong number of arguments"; goto loaderr;
        }
        for (j = 0; j < argc; j++)
            sdsfree(argv[j]);
        zfree(argv);
        sdsfree(line);
    }
    fclose(fp);
    return;

loaderr:
    fprintf(stderr, "\n*** FATAL CONFIG FILE ERROR ***\n");
    fprintf(stderr, "Reading the configuration file, at line %d\n", linenum);
    fprintf(stderr, ">>> '%s'\n", line);
    fprintf(stderr, "%s\n", err);
    exit(1);
}


/**
 * 监听client请求，建立连接，调用 createClient()
 */
static void acceptHandler(aeEventLoop *el, int fd, void *privdata, int mask) {
    REDIS_NOTUSED(el); REDIS_NOTUSED(mask); REDIS_NOTUSED(privdata);
    /* client ip and port, anetAccept will assign its value */
    char cip[128];
    int cport;
    int cfd = anetAccept(server.neterr, fd, cip, &cport);
    if (cfd == ANET_ERR) {
        redisLog(REDIS_DEBUG, "Accepting client connection: %s", server.neterr);
        return;
    }
    redisLog(REDIS_DEBUG, "Accepted %s:%d", cip, cport);
    if (createClient(cfd) == NULL) {
        redisLog(REDIS_WARNING, "Error allocating resource for the client");
        close(cfd); // May be already closed, just ingore errors
        return;
    }
    server.stat_numconnections++;
}

/* ======================= Main ======================= */
static void daemonize() {
    // parent exits
    if (fork() != 0) {
        exit(0);
    }
    // create a new session
    setsid();

    int fd = open("/dev/null", O_RDWR, 0);
    if (fd != -1) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > STDERR_FILENO) {
            close(fd);
        }
    }
    // Try to write the pid file
    FILE *fp = fopen("/var/run/redis.pid", "w");
    if (fp != NULL) {
        fprintf(fp, "%d\n", getpid());
        fclose(fp);
    }
}

int main(int argc, char **argv) {
    // 1. 初始化、加载 server config
    initServerConfig();
    if (argc == 2) {
        ResetServerSaveParams();
        loadServerConfig(argv[1]);
    } else if (argc > 2) {
        fprintf(stderr, "Usage: ./redis-server [/path/to/redis.conf]");
        exit(1);
    }

    // 2. init server
    initServer();
    if (server.daemonize) {
        daemonize();
    }
    redisLog(REDIS_NOTICE, "Server started, Redis version " REDIS_VERSION);

    // 3. 加载之前的数据
    if (loadDb(server.dbfilename) == REDIS_OK) {
        redisLog(REDIS_NOTICE, "DB loaded from disk");
    }

    // 4. 创建接受连接的 file event: 接受客户端的请求，建立连接，然后调用 createClient
    if (aeCreateFileEvent(server.el, server.fd, AE_READABLE, acceptHandler, NULL, NULL) == AE_ERR) {
        oom("creating file event");
    }
    redisLog(REDIS_NOTICE, "The server is now ready to accept connections");
    
    // 5. 启动
    aeMain(server.el);

    // 6. 删除
    aeDeleteEventLoop(server.el);
    return 0;
}