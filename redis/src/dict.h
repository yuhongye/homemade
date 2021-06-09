#ifndef _DICT_H
#include <stdbool.h>

#define _DICT_H

#define DICT_OK 0
#define DICT_ERR 1

/* Unused arguments generate annoying warning... */
#define DICT_NOTUSED(V) ((void) V)

typedef struct dictEntry {
    void *key;
    void *val;
    struct dictEntry *next;
} dictEntry;

/**
 * 不同类型需要相应的方法
 */
typedef struct dictType {
    unsigned int (*hashFunction)(const void *key);
    void *(*keyDup)(void *privdata, const void *key);
    void *(*valDup)(void *privdata, const void *obj);
    bool (*keyCompare)(void *privdata, const void *key1, const void *key2);
    void (*keyDestructor)(void *privdata, void *key);
    void (*valDestructor)(void *privdata, void *obj);
} dictType;

typedef struct dict {
    dictEntry **table;
    dictType *type;
    // table size, 2^n
    unsigned int size;
    // size - 1
    unsigned int sizemask;
    unsigned int used;
    // TODO: 这个字段是干嘛用的
    void *privdata;
} dict;

typedef struct dictIterator {
    dict *ht;
    int index;
    dictEntry *entry, *nextEntry;
} dictIterator;

/* hash table 的初始大小 */
#define DICT_HT_INITIAL_SIZE 16

/** 使用宏实现的功能 */
#define dictFreeEntryVal(ht, entry) \
    if ((ht)->type->valDestructor != NULL) \
        (ht)->type->valDestructor((ht)->privdata, (entry)->val)

#define dictSetHashVal(ht, entry, _val_) do { \
    if ((ht)->type->valDup != NULL) \
        entry->val = (ht)->type->valDup((ht)->privdata, _val_); \
    else \
        entry->val = (_val_); \
} while (false)

#define dictFreeEntryKey(ht, entry) \
if ((ht)->type->keyDestructor != NULL) \
    (ht)->type->keyDestructor((ht)->privdata, (entry)->key)

#define dictSetHashKey(ht, entry, _key_) do { \
    if ((ht)->type->keyDup != NULL) \
        entry->key = (ht)->type->keyDup((ht)->privdata, _key_); \
    else \
        entry->key = (_key_); \
} while (false)

#define dictCompareHashKeys(ht, key1, key2) \
    (((ht)->type->keyCompare != NULL) ? \
        (ht)->type->keyCompare((ht)->privdata, key1, key2) : \
        (key1) == (key2))

#define dictHashKey(ht, key) (ht)->type->hashFunction(key)

#define dictGetEntryKey(he) ((he)->key)
#define dictGetEntryVal(he) ((he)->val)
#define dictGetHashTableSize(ht) ((ht)->size)
#define dictGetHashTableUsed(ht) ((ht)->used)

/** API */
/**
 * 创建一个空的 hash table
 */
dict *dictCreate(dictType *type, void *privadata);

/**
 * 扩容
 * @param size 要扩容的大小，实际会向上取2^n
 */
int dictExpand(dict *ht, unsigned int size);

/**
 * @reurn 如果 key 已经存在则返回 DICT_ERR
 */
int dictAdd(dict *ht, void *key, void *val);

/**
 * 先调用 dictAdd 尝试插入，失败的话说明 key 已经存在，则 update
 */
int dictReplace(dict *ht, void *key, void *val);

/**
 * 删除指定的 key，并调用 keyDestructor 和 valueDestructor 释放 key value
 */
int dictDelete(dict *ht, const void *key);

/**
 * 删除指定的 key，但是不调用 keyDestructor 和 valueDestructor
 */
int dictDeleteNoFree(dict *ht, const void *key);

/**
 * 清空 hashtable, 但是 ht 依然存在
 */
void dictEmpty(dict *ht);

/**
 * 清空整个 hashtable, 并释放 ht
 */
void dictRelease(dict *ht);


dictEntry * dictFind(dict *ht, const void *key);

/**
 * 调用 dictExpand(ht, ht->used)
 */
int dictResize(dict *ht);

/** Iterator */
dictIterator *dictGetIterator(dict *ht);
dictEntry *dictNext(dictIterator *iter);
void dictReleaseIterator(dictIterator *iter);

dictEntry *dictGetRandomKey(dict *ht);

void dictPrintStats(dict *ht);

unsigned int dictGenHashFunction(const unsigned char *buf, int len);

/** Hash table types */
extern dictType dictTypeHeapStringCopyKey;
extern dictType dictTypeHeapStrings;
extern dictType dictTypeHeapStringCopyKeyValue;

#endif