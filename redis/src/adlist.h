#ifndef _ADLIST_H_
#define _ADLIST_H_

/** Node, List, and Iterator are the only data structures used currently. */

/**
 * 双向链表的节点
 */
typedef struct listNode {
    struct listNode *prev;
    struct listNode *next;
    void *value;
} listNode;

/**
 * 双向链表
 * head 和 tail 存储数据吗? 存储
 */
typedef struct list {
    listNode *head;
    listNode *tail;

    /** 指向函数的指针 */
    void *(*dup)(void *ptr);
    void (*free)(void *ptr);
    // 用来标胶是否相等
    int (*match)(void *ptr, void *key);

    unsigned int len;
} list;

/**
 *  双向遍历
 */ 
typedef struct listIter {
    listNode *next;
    listNode *prev;
    int direction;
} listIter;

#define AL_START_HEAD 0
#define AL_START_TAIL 1

/* 使用宏实现的功能 */
#define listLength(l) ((l)->len)
#define listFirst(l) ((l)->head)
#define listLast(l) ((l)->tail)
#define listPrevNode(n) ((n)->prev)
#define listNextNode(n) ((n)->next)
#define listNodeValue(n) ((n)->value)

#define listSetDupMethod(l, m) ( (l)->dup = (m) )
#define listSetFreeMethod(l, m) ( (l)->free = (m) )
#define listSetMatchMethod(l, m) ( (l)->match = (m) )

#define listGetDupMethod(l) ((l)->dup)
#define listGetFreeMethod(l) ((l)->free)
#define listGetMatchMethod(l) ((l)->match)

/* 原型函数 */

/**
 * 
 */
list *listCreate(void);

void listRelease(list *list);

list *listAddNodeHead(list *list, void *value);
list *listAddNodeTail(list *list, void *value);

/**
 * todo: 删除第一个匹配的元素 还是 删除全部匹配的元素 
 */
void listDelNode(list *list, listNode *node);

listIter *listGetIterator(list *list, int direction);
listNode *listNextElement(listIter *iter);
void listReleaseIterator(listIter *iter);

/**
 * 基于 orig 创建一个新的 list
 */ 
list *listDup(list *orig);

listNode *listSearchKey(list *list, void *key);
listNode *listIndex(list *list, int index);

#endif