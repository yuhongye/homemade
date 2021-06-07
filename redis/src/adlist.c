#include <stdio.h>
#include <stdlib.h>
#include "adlist.h"
#include "zmalloc.h"


/**
 * 
 * 
 */
list *listCreate() {
    struct list *list = zmalloc(sizeof(struct list));
    if (list == NULL) {
        return NULL;
    }
    list->len = 0;
    return list;
}

void listRelease(list *list) {
    listNode *node = list->head;
    for (int i = 0; i < list->len; i++) {
        zfree(node->value);
        listNode *temp = node;
        node = node->next;
        zfree(temp);
    }
    zfree(list);
}

list *listAddNodeHead(list *list, void *value) {
    listNode *node = zmalloc(sizeof(struct listNode));
    if (node == NULL) {
        return list;
    }
    node->value = value;
    node->prev = NULL;
    node->next = list->head;
    
    if (list->head == NULL) {
        list->head = list->tail = node;
    } else {
        list->head->prev = node;
        list->head = node;
    }

    list->len++;
    return list;
}

list *listAddNodeTail(list *list, void *value) {
    listNode *node = zmalloc(sizeof(struct listNode));
    if (node == NULL) {
        return list;
    }
    node->value = value;
    node->next = NULL;
    node->prev = list->tail;

    if (list->tail == NULL) {
        list->head = list->tail = node;
    } else {
        list->tail->next = node;
        list->tail = node;
    }

    list->len++;
    return list;
}

/**
 * 假设 node 就是 list 中的 节点
 */
void listDelNode(list *list, listNode *node) {
    if (node != list->head && node != list->tail) {
        listNode *temp = node->next;
        node->prev->next = node->next;
        node->next->prev = node->prev;
        node->prev = node->next = NULL;
    } else {
        if (node == list->head) {
            list->head = node->next;
            if (list->head != NULL) {
                list->head->prev = NULL;
            }
            node ->next = NULL;
        }
        if (node == list->tail) {
            list->tail = node->prev;
            if (list->tail != NULL) {
                list->tail->next = NULL;
            }
            node->prev = NULL;
        }
    }
    list->len -= 1;
    // TODO: 要不要free
    free(node->value);
    free(node);
}

listIter *listGetIterator(list *list, int direction) {
    struct listIter *it = zmalloc(sizeof(struct listIter));
    if (it == NULL) {
        return NULL;
    }
    it->direction = direction;
    it->next = list->head;
    it->prev = list->tail;
    return it;
}

listNode *listNextElement(listIter *iter) {
    if (iter->direction == AL_START_HEAD) {
        listNode *node = iter->next;
        if (iter->next != NULL) {
            iter->next = iter->next->next;
        }
        return node;
    } else {
        listNode *node = iter->prev;
        if (iter->prev != NULL) {
            iter->prev = iter->prev->prev;
        }
        return node;
    }
}

void listReleaseIterator(listIter *iter) {
    zfree(iter);
}

/**
 * 基于 orig 创建一个新的 list
 */ 
list *listDup(list *orig) {
    list *newList = listCreate();
    newList->len = orig->len;
    listNode *node = orig->head;
    for (int i = 0; i < orig->len; i++) {
        listAddNodeTail(newList, node);
        node = node->next;
    }
    return newList;
}

/**
 * 返回第一个 key
 */
listNode *listSearchKey(list *list, void *key) {
    listNode *node = list->head;
    while (node != NULL) {
        if (list->match(key, node->value) != 0) {
            node = node->next;
        }
    }
    return node;
}

/**
 * todo: 根据 index 来决定是从前往后遍历还是从后往前遍历
 */
listNode *listIndex(list *list, int index) {
    listNode *node = list->head;
    for (int i = 0; i < index; i++) {
        node = node->next;
    }
    return node;
}

/********************************** for test ************************/
void *idup(void *ptr) {
    int *p = (int *)ptr;
    int *copy = zmalloc(sizeof(int));
    if (copy == NULL) {
        return NULL;
    }
    *copy = *p;
    return copy;
}

void ifree(void *ptr) {
    zfree(ptr);
}

int imatch(void *ptr, void *key) {
    int *k1 = (int *) ptr;
    int *k2 = (int *) key;
    return *k1 < *k2 ? -1 : (*k1 > *k2 ? 1 : 0);
}


int main() {
    list *list = listCreate();
    listSetDupMethod(list, idup);
    listSetFreeMethod(list, ifree);
    listSetMatchMethod(list, imatch);
    for (int i = 0; i < 10; i++) {
        int *p = zmalloc(sizeof(int));
        *p = i;
        listAddNodeTail(list, p);
    }
    printf("list size: %u\n", listLength(list));


    listIter *it = listGetIterator(list, AL_START_HEAD);
    listNode *node = NULL;
    while ((node = listNextElement(it)) != NULL) {
        printf("%d\n", *((int *)node->value));
    }
    listReleaseIterator(it);

    listRelease(list);

    return 0;
}

