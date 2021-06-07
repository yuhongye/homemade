#ifndef _SDS_H
#define _SDS_H

#include <sys/types.h>

typedef char* sds;

/**
 * buf 的占用空间: len + free + 1, +1 是因为最后要添加一个 '\0'
 */
struct sdshdr{
    long len;
    long free;
    /**
     * 柔性数组，buf字段不占用空间，它不计算在sizeof内， 比如给这个结构体申请了 sizeof(sdshdr) + n 个字节，
     * 后面的n个字节就是buf可以用来存储的空间
     */
    char buf[];
};

/**
 * 创建大小为 initlen 的sds，如果 init 不为空，则把 init 中 initlen 个字节拷贝到新创建的 sds 中.
 * @param init 如果 init 不为空 拷贝它的内容到新创建的sds，否则 sds 的数据全部置为 0
 * @param initlen sds 的初始大小
 * @return 没有剩余空间的 sds 
 */
sds sdsnewlen(const void *init, size_t initlen);
/** 下面三个方法都是通过调用 sdsnewlen 来实现的 */
sds sdsnew(const char *init);
sds sdsempty();
sds sdsdup(const sds s);

/** 下面两个函数分别返回 sdshdr 的 len 和 free 字段 */
size_t sdslen(const sds s);
size_t sdsavail(sds s);

/** 释放sds */
void sdsfree(sds s);

/**
 * 调用标准库的 strlen 来计算 buf 真正的长度，
 * 使用这个长度来更新 header 的 len 和 free
 */
void sdsupdatelen(sds s);

/**
 * 将 t 中 len 个字符追加到 s 中. 二进制安全版本
 */
sds sdscatlen(sds s, void *t, size_t len);

/**
 * sdscatlen(s, t, strlen(t))
 */
sds sdscat(sds s, char *t);

/**
 * 使用格式化来生成数据，把数据追加到 s 中
 */
sds sdscatprintf(sds s, const char *fmt, ...);

/**
 * 将 t 中 len 个字符覆盖 s
 */
sds sdscpylen(sds s, char *t, size_t len);

/**
 * sdscpylen(s, t, strlen(t))
 */
sds sdscpy(sds s, char *t);

/**
 * 去重 s 两端中的 cset 字符, 比如:
 * s    = abc123456cde
 * cset = edba
 * trim = c123456c  
 */
sds sdstrim(sds s, const char *cset);

/**
 * 保留 s[start, end) 部分的内容，其余部分清空
 * @param startInclude 起始位置, 可以是负数
 * @param endExclude 结束位置, 可以是负数
 */
sds sdsrange(sds s, long start, long end);

/** 大小写转化 */
void sdstolower(sds);
void sdstoupper(sds);

/**
 * @return 比较大小：逐字符比较，如果都相同则比较长度
 */
int sdscmp(sds s1, sds s2);

/**
 * 使用 sep 来切割 s, 得到 count 个 sds
 * Notice: 对于结尾部分是 sep 的情况, 最后一个 token 长度为0
 * @param s 待切割的字符串
 * @param len s 的长度
 * @param sep 分隔符
 * @param seplen 分隔符的长度
 * @param count 切分结果大小
 */
sds *sdssplitlen(char *s, int len, char *sep, int seplen, int *count);

#endif