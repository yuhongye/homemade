#include <stdio.h>
#include <stdlib.h>
#include "adlist.h"
#include "zmalloc.h"


/**
 * 创建一个新 list, 新创建的 list 可以被AlFreList()释放，但是每个节点的 value 需要用户自己手动释放
 * 
 * @return empty list pointer or NULL if error
 */
list *listCreate() {
    struct list *list = zmalloc(sizeof(struct list));
    if (list == NULL) {
        return NULL;
    }
    list->head = list->tail = NULL;
    list->dup = NULL;
    list->free = NULL;
    list->match = NULL;
    list->len = 0;
    return list;
}

void listRelease(list *list) {
    unsigned int len = list->len;
    listNode *current = list->head;
    listNode *next;
    for (unsigned int i = 0; i < len; i++) {
        next = current->next;
        if (list->free != NULL) {
            list->free(current->value);
        }
        zfree(current);
        current = next;
    }
    zfree(list);
}

/**
 * 在队头添加的一个元素.
 * 内存不足时返回 NULL，不会做任何操作(传进来的 list 仍然存在).
 * 成功时传进来的 list
 */
list *listAddNodeHead(list *list, void *value) {
    listNode *node = zmalloc(sizeof(struct listNode));
    if (node == NULL) {
        return NULL;
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
        return NULL;
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
    if (node->prev != NULL) {
        node->prev->next = node->next;
    } else {
        // node 就是 head
        list->head = node->next;
    }

    if (node->next != NULL) {
        node->next->prev = node->prev;
    } else {
        // node 就是 tail
        list->tail = node->prev;
    }
    
    if (list->free != NULL) {
        list->free(node->value);
    }
    zfree(node);
    list->len--;
}

listIter *listGetIterator(list *list, int direction) {
    struct listIter *it = zmalloc(sizeof(struct listIter));
    if (it == NULL) {
        return NULL;
    }
    if (direction == AL_START_HEAD) {
        it->next = list->head;
    } else {
        it->next = list->tail;
    }
    it->direction = direction;
    return it;
}

listNode *listNextElement(listIter *iter) {
    listNode *current = iter->next;
    if (current != NULL) {
        if (iter->direction == AL_START_HEAD) {
            iter->next = current->next;
        } else {
            iter->next = current->prev;
        }
    }
    return current;
}

void listReleaseIterator(listIter *iter) {
    zfree(iter);
}

/**
 * 基于 orig 创建一个新的 list, 内存不足时返回 NULL.
 * 如果 'Dup' 函数存在的话，新创建的 list node 是基于 原来数据的拷贝，否则
 * 就是共享 value.
 * orig都不会改变
 */ 
list *listDup(list *orig) {
    list *newList = listCreate();
    if (newList == NULL) {
        return NULL;
    }
    newList->dup = orig->dup;
    newList->free = orig->free;
    newList->match = orig->match;

    int len = orig->len;
    listNode *node = orig->head;
    for (int i = 0; i < len; i++) {
        void *newValue = orig->dup != NULL ? orig->dup(node->value) : node->value;
        if (newValue == NULL || listAddNodeTail(newList, newValue) == NULL) {
            listRelease(newValue);
            return NULL;
        }
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
        if (list->match != NULL) {
            if (list->match(node->value, key)) {
                return node;
            }
        } else if (node->value == key) {
            return node;
        }
    }
    return NULL;
}

/**
 * @param index 指定元素的位置；如果是正数则从头开始找，起始位置是0；如果是负数，则头尾部开始找，起始位置是-1
 */
listNode *listIndex(list *list, int index) {
    listNode *n;
    if (index < 0) {
        index = (-index) - 1;
        n = list->tail;
        while(index-- && n != NULL) {
            n = n->prev;
        }
    } else {
        n = list->head;
        while (index-- && n != NULL) {
            n = n->next;
        }
    }
    return n;
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

static void displayList(list *list) {
    listIter *it = listGetIterator(list, AL_START_HEAD);
    listNode *node = NULL;
    printf("[");
    while ((node = listNextElement(it)) != NULL) {
        printf("%d, ", *((int *)node->value));
    }
    printf("]\n");
    listReleaseIterator(it);

}

int mainlist() {
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
    displayList(list);

    int size;
    listNode  *node;
    while ((size = listLength(list)) != 0) {
        int index = rand() % size;
        node = list->head;
        for (int i = 1; i < index; i++) {
            node = node->next;
        }
        printf("Will delete node: %d\n", *((int *)node->value));
        listDelNode(list, node);
        printf("After delete node, the list: ");
        displayList(list);
    }

    listRelease(list);

    return 0;
}

