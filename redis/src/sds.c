#include "sds.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "zmalloc.h"

static void sdsOomAbort(void) {
    fprintf(stderr, "SDS: Out Of Memory (SDS_ABORT_ON_OOM defined)\n");
    abort();
}

static struct sdshdr *header(sds s) {
    return (void*)(s - sizeof(struct sdshdr));
}

/**
 * 创建大小为 initlen 的sds，如果 init 不为空，则把 init 中 initlen 个字节拷贝到新创建的 sds 中.
 * @param init 如果 init 不为空 拷贝它的内容到新创建的sds，否则 sds 的数据全部置为 0
 * @param initlen sds 的初始大小
 * @return 没有剩余空间的 sds 
 */
sds sdsnewlen(const void *init, size_t initlen) {
    struct sdshdr *sh = zmalloc(sizeof(struct sdshdr) + initlen + 1);
    // Notice: 为了代码简洁，这里没有判断 SDS_ABORT_ON_OOM 是否定义，而是直接终止
    if (sh == NULL) {
        sdsOomAbort();
    }
    sh->len = initlen;
    sh->free = 0;
    if (initlen > 0) {
        if (init != NULL) {
            // memcpy 函数一定会拷贝 initlen 个字节
            memcpy(sh->buf, init, initlen);
        } else {
            memset(sh->buf, 0, initlen);
        }
    }
    sh->buf[initlen] = '\0';
    return (char *)sh->buf;
}

sds sdsempty(void) {
    return sdsnewlen("", 0);
}

sds sdsnew(const char *init) {
    size_t initlen = (init == NULL) ? 0 : strlen(init);
    return sdsnewlen(init, initlen);
}

sds sdsdup(sds s) {
    return sdsnewlen(s, sdslen(s));
}

size_t sdslen(const sds s) {
    return header(s)->len;
}

size_t sdsavail(sds s) {
    return header(s)->free;
}

void sdsfree(sds s) {
    if (s != NULL) {
        zfree(s - sizeof(struct sdshdr));
    }
}

void sdsupdatelen(sds s) {
    struct sdshdr *sh = header(s);
    // 注意这里调用的是标准库的 strlen
    int reallen = strlen(s);
    sh->free += (sh->len - reallen);
    sh->len = reallen;
}

/**
 * 确保 s 中至少还有 addlen 的剩余空间，如果不够的话就 resize.
 * Notice: addlen 不包括结果的 '\0'
 */
static sds sdsMakeRoomFor(sds s, size_t addlen) {
    size_t free = sdsavail(s);
    if (free >= addlen) {
        return s;
    }

    size_t len = sdslen(s);
    struct sdshdr *sh = (void *) (s - sizeof(struct sdshdr));
    size_t newlen = (len + addlen) * 2;
    struct sdshdr *newsh = zrealloc(sh, sizeof(struct sdshdr) + newlen + 1);
    if (newsh == NULL) {
        sdsOomAbort();
    }
    newsh->free = newlen - len;
    return newsh->buf;
}

/**
 * 将 t 的内容追加到 s 后面，如果 s 的空间不够就 resize
 */
sds sdscatlen(sds s, void *t, size_t len) {
    size_t curlen = sdslen(s);
    s = sdsMakeRoomFor(s, len);
    // 在我们的设定中，s 不可能为 NULL，否则就 abort 了
    struct sdshdr *sh = (void *)(s - sizeof(struct sdshdr));
    memcpy(s+curlen, t, len);
    sh->len = curlen + len;
    sh->free = sh->free - len;
    s[curlen+len] = '\0';
    return s;
}

sds sdscat(sds s, char *t) {
    return sdscatlen(s, t, strlen(t));
}

/**
 * 将格式化的内容拷贝到 buf 中，然后将 buf 中的内容追加到 s
 */
sds sdscatprintf(sds s, const char *fmt, ...) {
    va_list ap;
    size_t buflen = 32;
    char *buf;
    while (true) {
        buf = zmalloc(buflen);
        if (buf == NULL) {
            sdsOomAbort();
        }
        // todo: 为什么是 -2 
        buf[buflen-2] = '\0';
        va_start(ap, fmt);
        vsnprintf(buf, buflen, fmt, ap);
        va_end(ap);
        if (buf[buflen-2] != '\0') {
            zfree(buf);
            buflen *= 2;
            continue;
        }
        break;
    }
    char *t = sdscat(s, buf);
    zfree(buf);
    return t;
}

/**
 * 用 t 来覆盖 s 的已有内容
 */
sds sdscpylen(sds s, char *t, size_t len) {
    struct sdshdr *sh = header(s);
    size_t totlen = sh->free + sh->len;
    if (totlen < len) {
        s = sdsMakeRoomFor(s, len-totlen);
        sh = header(s);
        totlen = sh->free + sh->len;
    }
    memcpy(s, t, len);
    s[len] = '\0';
    sh->len = len;
    sh->free = totlen - len;
    return s;
}

sds sdscpy(sds s, char *t) {
    return sdscpylen(s, t, strlen(t));
}

/**
 * 去除两端中指定的字符，比如: s = "Sds is a simply strings"
 *                         cset = "dsS"
 *                    返回结果: " is a simply string"
 */
sds sdstrim(sds s, const char *cset) {
    char *start, *end, *sp, *ep;
    sp = start = s;
    ep = end = s + sdslen(s) - 1;
    while (sp <= end && strchr(cset, *sp) != NULL) {
        sp++;
    }
    while (ep > start && strchr(cset, *ep) != NULL) {
        ep--;
    }
    size_t len = sp > ep ? 0 : (ep-sp) + 1;
    struct sdshdr *sh = header(s);
    if (sh->buf != sp) {
        // 保留 sp ep 之间的数据
        memmove(sh->buf, sp, len);
    }
    sh->buf[len] = '\0';
    sh->free = sh->free + (sh->len - len);
    sh->len = len;
    return s;
}

/**
 * 保留 s[start, end) 这一部分内容，其他都清除掉了
 * Notice:
 *  1. start 和 end 支持负数的形式
 *  2. 没有对 start 和 end 做合理性要求，代码部分做了兼容处理
 */
sds sdsrange(sds s, long start, long end) {
    size_t len = sdslen(s);
    if (len == 0) {
        return s;
    }

    if (start < 0) {
        start = len + start;
        if (start < 0) {
            start = 0;
        }
    }

    if (end < 0) {
        end = len + end;
        if (end < 0) {
            end = 0;
        }
    }

    size_t newlen = (start > end) ? 0 : (end - start) + 1;
    if (newlen != 0) {
        if (start >= (signed) len) {
            start = len - 1;
        }
        if (end >= (signed) len) {
            end = len - 1;
        }
        newlen = (start > end) ? 0 : (end - start) + 1;
    } else {
        start = 0;
    }

    struct sdshdr *sh = header(s);
    if (start != 0) {
        memmove(sh->buf, sh->buf + start, newlen);
    }
    sh->buf[newlen] = 0;
    sh->free = sh->free + (sh->len - newlen);
    sh->len = newlen;
    return s;
}

void sdstolower(sds s) {
    int len = sdslen(s);
    for (int i = 0; i < len; i++) {
        s[i] = tolower(s[i]);
    }
}

void sdstoupper(sds s) {
    int len = sdslen(s);
    for (int i = 0; i < len; i++) {
        s[i] = toupper(s[i]);
    }
}

int sdscmp(sds s1, sds s2) {
    size_t l1 = sdslen(s1);
    size_t l2 = sdslen(s2);
    size_t minlen = l1 < l2 ? l1 : l2;
    int cmp = memcmp(s1, s2, minlen);
    return cmp == 0 ? l1 - l2 : cmp;
}

/**
 * 使用 sep 对 s 进行分割，分割的结果是 sds 数组，数组的大小保存在 count 中.
 * 如果 OOM 了，zero length string, zero length seprator, NULL is returned.
 * 
 * sep 可以是多个字符，比如 sdssplit("foo_-_bar", "_-_") 返回 "foo" 和 "bar"
 * Notice: 对于结尾部分是 sep 的情况, 最后一个 token 长度为0
 */
sds *sdssplitlen(char *s, int len, char *sep, int seplen, int *count) {
    if (seplen < 1 || len < 0) {
        return NULL;
    }

    int slots = 5;
    sds *tokens = zmalloc(sizeof(sds) * 5);
    if (tokens == NULL) {
        sdsOomAbort();
    }

    // 已经解析的 token 个数
    int elements = 0;
    // next start position
    int start = 0;
    for (int i = 0; i < (len - (seplen - 1)); i++) {
        // resize, why plus 2? for loop will add one token, loop end will add the final one
        if (slots < elements + 2) {
            slots *= 2;
            sds *newtokens = zrealloc(tokens, sizeof(sds) * slots);
            if (newtokens == NULL) {
                sdsOomAbort();
            }
            tokens = newtokens;
        }

        if (seplen == 1 && *(s+i) == sep[0] || (memcmp(s+i, sep, seplen) == 0)) {
            tokens[elements] = sdsnewlen(s+start, i-start);
            if (tokens[elements] == NULL) {
                sdsOomAbort();
            }
            elements++;
            start = i + seplen;
            // for loop will plus one
            i += seplen - 1;
        }
    }
    // 添加最后一个 element
    tokens[elements] = sdsnewlen(s + start, len - start);
    if (tokens[elements] == NULL) {
        sdsOomAbort();
    }
    *count = elements + 1;
    return tokens;
}

// for test
int main5() {
    sds s = sdsnew("Sds is Simple Dynamic String Sds");
    printf("%s\n", s);

    s = sdstrim(s, "dSs");
    printf("%s\n", s);

    s = sdsnew("Sds is Simple Dynamic String.");
    sdsrange(s, 0, 10);
    printf("%s\n", s);

    int count = 0;
    char *t = "a_b_c_d_e_f_g_";
    sds *array = sdssplitlen(t, strlen(t), "_", 1, &count);
    printf("count: %d\n", count);
    for (int i = 0; i < count; i++) {
        printf("\t%ld, %s\n", header(array[i])->len, array[i]);
    }

    count = 0;
    t = "a_@b_@c_@d_@";
    array = sdssplitlen(t, strlen(t), "_@", 2, &count);
    printf("count: %d\n", count);
    for (int i = 0; i < count; i++) {
        printf("\t%ld, %s\n", header(array[i])->len, array[i]);
    }

    return 0;
}

