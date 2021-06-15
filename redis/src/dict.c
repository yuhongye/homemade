#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "dict.h"
#include "zmalloc.h"

/** ------------------- Utility functions --------------------- */
static void _dictPanic(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    fprintf(stderr, "\nDICT LIBRARY PANIC: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n\n");
    va_end(ap);
}

/** ---------------- Heap Management Wrappers -------------- */
static void *_dictAlloc(int size) {
    void *p = zmalloc(size);
    if (p == NULL) {
        _dictPanic("Out Of Memory");
    }
    return p;
}

static void _dictFree(void *ptr) {
    zfree(ptr);
}

/* ----------------------- private prototypes ---------------- */
static int _dictExpandIfNeeded(dict *ht);
static unsigned int _dictNextPower(unsigned int size);
static int _dictKeyIndex(dict *ht, const void *key);
static int _dictInit(dict *ht, dictType *type, void *privDataPtr);

/* --------------------- hash functions --------------------- */
/* Thomas Wang's 32 bit Mix Function */
unsigned int dictIntHashFunction(unsigned int key) {
    key += ~(key << 15);
    key ^=  (key >> 10);
    key +=  (key << 3);
    key ^=  (key >> 6);
    key += ~(key << 11);
    key ^=  (key >> 16);
    return key;
}

/* Identity hash function for integer keys */
unsigned int dictIdentityHashFunction(unsigned int key) {
    return key;
}

/**
 * Generic hash function, 基本类似 Java Effective 推荐的 hash 函数实现
 */
unsigned int dictGenHashFunction(const unsigned char *buf, int len) {
    unsigned int hash = 5381;
    while(len-- > 0) {
        hash = ((hash << 5) + hash) + (*buf++);
    }
    return hash;
}

/* ----------------------- API Implementation ------------------ */
/**
 * 重置已经 调用 hl_init() 函数初始化过的 hash table
 */
static void _dictReset(dict *ht) {
    ht->table = NULL;
    ht->size = 0;
    ht->sizemask = 0;
    ht->used = 0;
}

int _dictInit(dict *ht, dictType *type, void *privdataPtr) {
    _dictReset(ht);
    ht->type = type;
    ht->privdata = privdataPtr;
    return DICT_OK;
}

dict *dictCreate(dictType *type, void *privDataPtr) {
    dict *ht = _dictAlloc(sizeof(*ht));
    _dictInit(ht, type, privDataPtr);
    return ht;
}

/**
 * resize or create the hash table
 * @param ht hash table, 扩容后 ht 指针不变
 * @param size 要求的最小 slot size
 * @return 0 if success, otherwise 1
 */
int dictExpand(dict *ht, unsigned int size) {
    // 扩容的最小大小要覆盖已有元素的个数
    if (ht->used > size) {
        return DICT_ERR;
    }
    unsigned int realsize = _dictNextPower(size);
    printf("resize realsize: %u\n", realsize);
    // remain: n is not pointer; the new hashtable
    dict n;
    _dictInit(&n, ht->type, ht->privdata);
    n.size = realsize;
    n.sizemask = realsize - 1;
    n.table = _dictAlloc(realsize * sizeof(dictEntry *));
    // table slot to null
    memset(n.table, 0, realsize * sizeof(dictEntry*));

    // rehash
    n.used = ht->used;
    for (int i = 0; i < ht->size && ht->used > 0; i++) {
        // empty slot
        if (ht->table[i] == NULL) {
            continue;
        }
        dictEntry *entry, *nextEntry;
        entry = ht->table[i];
        while (entry != NULL) {
            nextEntry = entry->next;
            unsigned int newSlot = dictHashKey(ht, entry->key) & n.sizemask;
            // 头插法
            entry->next = n.table[newSlot];
            n.table[newSlot] = entry;
            ht->used--;
            entry = nextEntry;
        }
    }
    assert(ht->used == 0);
    _dictFree(ht->table);
    *ht = n;
    return DICT_OK;
}

/**
 * resize table, 要求的最小 size 是 hash table 中已有元素的个数
 */
int dictResize(dict *ht) {
    int minimal = ht->used;
    if (minimal < DICT_HT_INITIAL_SIZE) {
        minimal = DICT_HT_INITIAL_SIZE;
    }
    return dictExpand(ht, minimal);
}

/**
 * hashtable 新增一个 kv
 * @return 1 if key already exist, 0 if add success
 */
int dictAdd(dict *ht, void *key, void *val) {
    int index = _dictKeyIndex(ht, key);
    if (index == -1) {
        return DICT_ERR;
    }

    dictEntry *entry = _dictAlloc(sizeof(*entry));
    entry->next = ht->table[index];
    ht->table[index] = entry;

    dictSetHashKey(ht, entry, key);
    dictSetHashVal(ht, entry, val);
    ht->used++;
    return DICT_OK;
}

int dictReplace(dict *ht, void *key, void *val) {
    if (dictAdd(ht, key, val) == DICT_OK) {
        return DICT_OK;
    }

    dictEntry *entry = dictFind(ht, key);
    // free old value
    dictFreeEntryVal(ht, entry);
    dictSetHashVal(ht, entry, val);
    return DICT_OK;
}

/**
 * 删除指定 key 
 * @param nofree 是否释放
 */
static int dictGenericDelete(dict *ht, const void *key, int nofree) {
    if (ht->size == 0) {
        return DICT_ERR;
    }

    unsigned int slot = dictHashKey(ht, key) & ht->sizemask;
    dictEntry *entry = ht->table[slot];
    dictEntry *prevEntry = NULL;
    while (entry != NULL) {
        if (dictCompareHashKeys(ht, key, entry->key)) {
            if (prevEntry != NULL) {
                prevEntry->next = entry->next;
            } else {
                // 桶中的第一个元素
                ht->table[slot] = entry->next;
            }
            if (!nofree) {
                dictFreeEntryKey(ht, entry);
                dictFreeEntryVal(ht, entry);
            }

            _dictFree(entry);
            ht->used--;
            return DICT_OK;
        }
        prevEntry = entry;
        entry = entry->next;
    }
    return DICT_ERR;
}

/**
 * 删除指定的 key，并调用 key
 */
int dictDelete(dict *ht, const void *key) {
    return dictGenericDelete(ht, key, false);
}

int dictDeleteNoFree(dict *ht, const void *key) {
    return dictGenericDelete(ht, key, true);
}

dictEntry *dictFind(dict *ht, const void *key) {
    if (ht->size == 0) {
        return NULL;
    }
    unsigned int slot = dictHashKey(ht, key) & ht->sizemask;
    dictEntry *entry = ht->table[slot];
    while(entry !=  NULL) {
        if (dictCompareHashKeys(ht, key, entry->key)) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

/** destroy an entire hash table */
int _dictClear(dict *ht) {
    for (int i = 0; i < ht->size && ht->used > 0; i++) {
        dictEntry *entry = ht->table[i];
        dictEntry *nextEntry = NULL;
        while (entry != NULL) {
            nextEntry = entry->next;
            dictFreeEntryKey(ht, entry);
            dictFreeEntryVal(ht, entry);
            _dictFree(entry);
            ht->used--;
            entry = nextEntry;
        }
    }

    _dictFree(ht->table);
    _dictReset(ht);
    return DICT_OK;
}

void dictRelease(dict *ht) {
    _dictClear(ht);
    _dictFree(ht);
}

/** Iterator */
dictIterator *dictGetIterator(dict *ht) {
    dictIterator *it = _dictAlloc(sizeof(*it));
    it->ht = ht;
    it->index = -1;
    it->entry = it->nextEntry = NULL;
    return it;
}

dictEntry *dictNext(dictIterator *it) {
    while (true) {
        if (it->entry == NULL) {
            it->index++;
            if (it->index >= (signed) it->ht->sizemask) {
                break;
            }
            it->entry = it->ht->table[it->index];
        } else {
            it->entry = it->nextEntry;
        }

        if (it->entry != NULL) {
            // 需要保存下一个，因为返回的entry有可能被删掉
            it->nextEntry = it->entry->next;
            return it->entry;
        }
    }
    return NULL;
}

void dictReleaseIterator(dictIterator *it) {
    _dictFree(it);
}

dictEntry *dictGetRandomKey(dict *ht) {
    if (ht->size == 0) {
        return NULL;
    }
    dictEntry *entry;
    unsigned int slot;
    do {
        slot = random() & ht->sizemask;
        entry = ht->table[slot];
    } while (entry == NULL);

    /**
     * 现在找到了一个非空的 slot，slot 是一个 linked list
     */
    int listen = 0;
    while (entry != NULL) {
        entry = entry->next;
        listen++;
    }
    int listele = random() % listen;
    entry = ht->table[slot];
    while (listele-- > 0) {
        entry = entry->next;
    }
    return entry;
}

void dictEmpty(dict *ht) {
    _dictClear(ht);
}


/* ----------------------- private functions ----------------------- */

/**
 * 如果 hashtable 为空则新建一个; 如果 hashtable 满了就扩容
 */
static int _dictExpandIfNeeded(dict *ht) {
    if (ht->size == 0) {
        return dictExpand(ht, DICT_HT_INITIAL_SIZE);
    }

    if (ht->used == ht->size) {
        return dictExpand(ht, ht->size * 2);
    }
    return DICT_OK;
}

static unsigned int _dictNextPower(unsigned int size) {
    if (size >= 2147483648U) {
        return 2147483648U;
    }
    unsigned int i = DICT_HT_INITIAL_SIZE;
    while (true) {
        if (i >= size) {
            return i;
        }
        i *= 2;
    }
}

/**
 * 会自动扩容
 * 找到 key 要插入的 slot, 如果 key 已经存在则返回 -1
 */
static int _dictKeyIndex(dict *ht, const void *key) {
    if (_dictExpandIfNeeded(ht) == DICT_ERR) {
        return -1;
    }
    unsigned int slot = dictHashKey(ht, key) & ht->sizemask;
    dictEntry *entry = ht->table[slot];
    while (entry != NULL) {
        // todo: == 0 or != 0
        if (dictCompareHashKeys(ht, key, entry->key) != 0) {
            return -1;
        }
        entry = entry->next;
    }
    return slot;
}

#define DICT_STATS_VECTLEN 50
void dictPrintStats(dict *ht) {
    if (ht->used == 0) {
        printf("No stats available for empty dictionaries\n");
        return;
    }

    // 有数据的slots
    unsigned int slots = 0;
    unsigned int maxchainlen = 0;
    unsigned int totchainlen = 0;
    // 每种链表长度的个数
    unsigned int clvector[DICT_STATS_VECTLEN];
    for (int i = 0; i < DICT_STATS_VECTLEN; i++) {
        clvector[i] = 0;
    }
    for (int i = 0; i < ht->size; i++) {
        dictEntry *entry = ht->table[i];
        if (entry == NULL) {
            clvector[0]++;
            continue;
        }
        slots++;
        unsigned int chainlen = 0;
        while(entry != NULL) {
            chainlen++;
            entry = entry->next;
        }
        clvector[(chainlen < DICT_STATS_VECTLEN) ? chainlen : (DICT_STATS_VECTLEN-1)]++;
        if (chainlen > maxchainlen) {
            maxchainlen = chainlen;
        }
        totchainlen += chainlen;
    }
    printf("Hash table stats: \n");
    printf(" table size: %d\n", ht->size);
    printf(" number of elements: %d\n", ht->used);
    printf(" different slots: %d\n", slots);
    printf(" max chain length: %d\n", maxchainlen);
    printf(" avg chain length (counted): %.02f\n", (float)totchainlen / slots);
    printf(" avg chain length (computed): %.02f\n", (float)ht->used / slots);
    printf(" Chain length distribution: \n");
    for (int i = 0; i < DICT_STATS_VECTLEN-1; i++) {
        if (clvector[i] == 0) continue;
        printf("   %s%d: %d (%.02f%%)\n",(i == DICT_STATS_VECTLEN-1)?">= ":"", i, clvector[i], ((float)clvector[i]/ht->size)*100);
    }
}

/* -------------------------- StringCopy Hash Table Type -------------------*/
static unsigned int _dictStringCopyHTHashFunction(const void *key) {
    return dictGenHashFunction(key, strlen(key));
}

/**
 * string copy
 */
static void *_dictStringCopyHTKeyDup(void *privdata, const void *key) {
    int len = strlen(key);
    char *copy = _dictAlloc(len + 1);
    DICT_NOTUSED(privdata);

    memcpy(copy, key, len);
    copy[len] = '\0';
    return copy;
}

static void *_dictStringKeyValCopyHTValDup(void *privdata, const void *val){
    int len = strlen(val);
    char *copy = _dictAlloc(len+1);
    DICT_NOTUSED(privdata);

    memcpy(copy, val, len);
    copy[len] = '\0';
    return copy;
}

static bool _dictStringCopyHTKeyCompare(void *privdata, const void *key1, const void *key2) {
    DICT_NOTUSED(privdata);
    return strcmp(key1, key2) == 0;
}

static void _dictStringCopyHTKeyDestructor(void *privdata, void *key) {
    DICT_NOTUSED(privdata);
    _dictFree((void *) key); // ATTENTION: const cast
}

static void _dictStringKeyValCopyHTValDestructor(void *privdata, void *val){
    DICT_NOTUSED(privdata);
    _dictFree((void*)val); /* ATTENTION: const cast */
}

dictType dictTypeHeapStringCopyKey = {
    _dictStringCopyHTHashFunction,        /* hash function */
    _dictStringCopyHTKeyDup,              /* key dup */
    NULL,                               /* val dup */
    _dictStringCopyHTKeyCompare,          /* key compare */
    _dictStringCopyHTKeyDestructor,       /* key destructor */
    NULL                                /* val destructor */
};

/* This is like StringCopy but does not auto-duplicate the key.
 * It's used for intepreter's shared strings. */
dictType dictTypeHeapStrings = {
    _dictStringCopyHTHashFunction,        /* hash function */
    NULL,                               /* key dup */
    NULL,                               /* val dup */
    _dictStringCopyHTKeyCompare,          /* key compare */
    _dictStringCopyHTKeyDestructor,       /* key destructor */
    NULL                                /* val destructor */
};

/* This is like StringCopy but also automatically handle dynamic
 * allocated C strings as values. */
dictType dictTypeHeapStringCopyKeyValue = {
    _dictStringCopyHTHashFunction,        /* hash function */
    _dictStringCopyHTKeyDup,              /* key dup */
    _dictStringKeyValCopyHTValDup,        /* val dup */
    _dictStringCopyHTKeyCompare,          /* key compare */
    _dictStringCopyHTKeyDestructor,       /* key destructor */
    _dictStringKeyValCopyHTValDestructor, /* val destructor */
};


/* Hash table types */
extern dictType dictTypeHeapStringCopyKey;
extern dictType dictTypeHeapStrings;
extern dictType dictTypeHeapStringCopyKeyValue;

/* ---------------------- for test ------------------ */
int main() {
    dict *ht = dictCreate(&dictTypeHeapStringCopyKeyValue, NULL);
    for (int i = 0; i < 1025; i++) {
        char *key = _dictAlloc(8);
        char *value = _dictAlloc(10);
        sprintf(key, "key%d", i);
        sprintf(value, "value%d", i);
        dictAdd(ht, key, value);
    }

    dictIterator *it = dictGetIterator(ht);
    dictEntry *entry;
    while ((entry = dictNext(it)) != NULL) {
        printf("%s: %s\n", entry->key, entry->val);
    }
    dictReleaseIterator(it);

    dictPrintStats(ht);
    return 0;
}