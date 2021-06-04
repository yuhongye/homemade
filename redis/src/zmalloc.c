#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static size_t used_memory = 0;

/**
 * 额外申请 sizeof(size_t) 的内存 用来保存本次申请的大小
 */
void *zmalloc(size_t size) {
    size_t cap = size + sizeof(size_t);
    void *ptr = malloc(cap);
    // todo: 这里不需要判断 ptr 是否为 NULL 吗
    *((size_t*) ptr) = size;
    used_memory += cap;
    return ptr + sizeof(size_t);
}

/**
 * 扩缩容
 */
void *zrealloc(void *ptr, size_t size) {
    if (ptr == NULL) {
        return zmalloc(size);
    }

    void *realptr = ptr - sizeof(size_t);
    // 获取申请 ptr 时的大小
    size_t oldsize = *((size_t*) realptr);
    void *newptr = realloc(realptr, size + sizeof(size_t));
    if (newptr == NULL) {
        return NULL;
    }

    *((size_t*) newptr) = size;
    used_memory -= oldsize;
    // 这里不需要再加 sizeof(size_t), 因此在申请 ptr 时已经加过了
    used_memory += size;
    return newptr + sizeof(size_t);
}

void zfree(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    void *realptr = ptr - sizeof(size_t);
    size_t oldsize = *((size_t*)realptr);
    used_memory -= oldsize + sizeof(size_t);
    free(realptr);
}

char *zstrdup(const char *s) {
    size_t l = strlen(s) + 1;
    char *p = zmalloc(l);

    memcpy(p, s, l);
    return p;
}

size_t zmalloc_used_memory(void) {
    return used_memory;
}

static size_t ptrSize(void *ptr) {
    if (ptr == NULL) {
        return 0;
    }

    return *((size_t*) (ptr - sizeof(size_t)));
}

// for test
int main() {
    void *p = zmalloc(8);
    printf("Point address: %p, size: %zu\n", p, ptrSize(p));
    *((int*)p) = 1024;
    printf("Value: %d\n", *((int*)p));

    zfree(p);

    return 0;
}