#ifndef __AE_H__
#include <stdbool.h>

#define __AE_H__

/**
 * 事件分成 file event and time event
 */

struct aeEventLoop;

typedef void aeFileProc(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);
typedef int aeTimeProc(struct aeEventLoop *eventLoop, long long id, void *clientData);
typedef void *aeEventFinalizerProc(struct aeEventLoop *eventLoop, void *clientData);

typedef struct aeFileEvent {
    int fd;
    int mask; // AE_(READABLE|WRITABLE|EXCEPTION)
    aeFileProc *fileProc;
    aeEventFinalizerProc *finalizerProc;
    void *clientData;
    struct aeFileEvent *next;
} aeFileEvent;

typedef struct aeTimeEvent {
    long long id; // time event identifier
    long when_sec;
    long when_ms;
    aeTimeProc *timeProc;
    aeEventFinalizerProc *finalizerProc;
    void *clientData;
    struct aeTimeEvent *next;
} aeTimeEvent;

typedef struct aeEventLoop {
    long long timeEventNextId;
    aeFileEvent *fileEventHead;
    aeTimeEvent *timeEventHead;
    // bool 本质上是一个 unsigned int, 在 redis 的代码中的类型是 int
    bool stop;
} aeEventLoop;


/** Defines */
#define AE_OK 0
#define AE_ERR -1

#define AE_READABLE 1
#define AE_WRITABLE 2
#define AE_EXCEPTION 4

#define AE_FILE_EVENTS 1
#define AE_TIME_EVENTS 2
#define AE_ALL_EVENTS (AE_FILE_EVENTS | AE_TIME_EVENTS)
#define AE_DONT_WAIT 4

#define AE_NOMORE -1

/** Macros */
#define AE_NOTUSED(V) ((void) V)

#define isFileEventSet(flags) (((flags) & AE_FILE_EVENTS) != 0)
#define isTimeEventSet(flags) (((flags) & AE_FILE_EVENTS) != 0)
#define isDontWaitSet(flags) (((flags) & AE_DONT_WAIT) != 0)

#define isReadable(mask) (((mask) & AE_READABLE) != 0)
#define isWritable(mask) (((mask) & AE_WRITABLE) != 0)
#define isException(mask) (((mask) & AE_EXCEPTION) != 0)

/** API */
aeEventLoop *aeCreateEventLoop(void);
void aeDeleteEventLoop(aeEventLoop *eventLoop);
void aeStop(aeEventLoop *eventLoop);

/**
 * 创建 file event，并且插入到 eventLoop 的队头
 */
int aeCreateFileEvent(aeEventLoop *eventLoop, int fd, int mask,
                      aeFileProc *proc, void *clientData, aeEventFinalizerProc *finalizerProc);
// 从 eventLoop 中删除 file event
void aeDeleteFileEvent(aeEventLoop *eventLoop, int fd, int mask);

/** 创建 time event 插入到 event loop 中； 从 eventLoop 中删除 time event */
long long aeCreateTimeEvent(aeEventLoop *eventLoop, long long milliseconds, 
        aeTimeProc *proc, void *clientData,
        aeEventFinalizerProc *finalizerProc);
int aeDeleteTimeEvent(aeEventLoop *eventLoop, long long id);

/**
 * 最重要的方法，开始处理事件
 * @param flags: 0 or AE_FILE_EVENTS or AE_TIME_EVENTS or AE_ALL_EVENTS or AE_DONT_WAIT
 */
int aeProcessEvents(aeEventLoop *eventLoop, int flags);

int aeWait(int fd, int mask, long long milliseconds);

/**
 * 在 eventLoop 没有 stop 前不停的调用 aeProcessEvents
 */
void aeMain(aeEventLoop *eventLoop);

#endif