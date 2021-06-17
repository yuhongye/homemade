#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "ae.h"
#include "zmalloc.h"

/** event loop create, delete, stop */
aeEventLoop *aeCreateEventLoop(void) {
    aeEventLoop *eventLoop = zmalloc(sizeof(struct aeEventLoop));
    if (eventLoop == NULL) {
        return NULL;
    }

    eventLoop->fileEventHead = NULL;
    eventLoop->timeEventHead = NULL;
    eventLoop->timeEventNextId = 0;
    eventLoop->stop = false;
    return eventLoop;
}

void aeDeleteEventLoop(aeEventLoop *eventLoop) {
    zfree(eventLoop);
}

void aeStop(aeEventLoop *eventLoop) {
    eventLoop->stop = true;
}

/************************* file event create and delete *************************/
int aeCreateFileEvent(aeEventLoop *eventLoop, int fd, int mask, aeFileProc *proc, void *clientData, aeEventFinalizerProc *finalizerProc) {
    aeFileEvent *fe = zmalloc(sizeof(struct aeFileEvent));
    if (fe == NULL) {
        return AE_ERR;
    }
    fe->fd = fd;
    fe->mask = mask;
    fe->fileProc = proc;
    fe->finalizerProc = finalizerProc;
    fe->clientData = clientData;
    fe->next = eventLoop->fileEventHead;
    eventLoop->fileEventHead = fe;
    return AE_OK;
}

void aeDeleteFileEvent(aeEventLoop *eventLoop, int fd, int mask) {
    aeFileEvent *fe = eventLoop->fileEventHead;
    aeFileEvent *prev = NULL;
    while (fe != NULL) {
        if (fe->fd == fd && fe->mask == mask) {
            if (prev == NULL) {
                eventLoop->fileEventHead = fe->next;
            } else {
                prev->next = fe->next;
            }
            if (fe->finalizerProc != NULL) {
                fe->finalizerProc(eventLoop, fe->clientData);
            }
            zfree(fe);
            return;
        }
        prev = fe;
        fe = fe->next;
    }
}

/************************** time event 相关函数 ***********************/

/**
 * 将当前时间设置到 seconds 和 milliseconds 中
 */ 
static void aeGetTime(long *seconds, long *milliseconds) {
    struct timeval tv;
    ///               time zone
    gettimeofday(&tv, NULL);
    *seconds = tv.tv_sec;
    *milliseconds = tv.tv_usec / 1000;
}

/**
 * 在当前时间点加上 milliseconds，把结果保存到 sec 和 ms 中
 */
static void aeAddMillisecondsToNow(long long milliseconds, long *sec, long *ms) {
    long cur_sec, cur_ms;
    aeGetTime(&cur_sec, &cur_ms);
    long when_sec = cur_sec + milliseconds / 1000;
    long when_ms = cur_ms + milliseconds % 1000;
    if (when_ms >= 1000) {
        when_sec++;
        when_ms -= 1000;
    }

    *sec = when_sec;
    *ms = when_ms;
}

/**
 * @param milliseconds 触发事件的延迟时间
 * @return time event id 
 */
long long aeCreateTimeEvent(aeEventLoop *eventLoop, long long milliseconds, aeTimeProc *proc, void *clientData, aeEventFinalizerProc *finalizerProc) {
    long long id = eventLoop->timeEventNextId++;
    aeTimeEvent *te = zmalloc(sizeof(struct aeTimeEvent));
    if (te == NULL) {
        return AE_ERR;
    }

    te->id = id;
    aeAddMillisecondsToNow(milliseconds, &te->when_sec, &te->when_ms);
    te->timeProc = proc;
    te->finalizerProc = finalizerProc;
    te->clientData = clientData;
    te->next = eventLoop->timeEventHead;
    eventLoop->timeEventHead = te;
    return id;
}

int aeDeleteTimeEvent(aeEventLoop *eventLoop, long long id) {
    aeTimeEvent *te = eventLoop->timeEventHead;
    aeTimeEvent *prev = NULL;
    while (te != NULL) {
        if (te->id == id) {
            if (prev == NULL) {
                eventLoop->timeEventHead = te->next;
            } else {
                prev->next = te->next;
            }
            zfree(te);
            return AE_OK;
        }
        prev = te;
        te = te->next;
    }
    return AE_ERR; 
}

/**
 * 遍历一遍 time event list, 找到时间最小的那个 event
 * 时间复杂度: O(N), time event list 未排序，所以需要遍历一遍
 */
static aeTimeEvent *aeSearchNearestTimer(aeEventLoop *eventLoop) {
    aeTimeEvent *te = eventLoop->timeEventHead;
    aeTimeEvent *nearest = NULL;
    while (te != NULL) {
        if (nearest == NULL || te->when_sec < nearest->when_sec || (te->when_sec == nearest->when_sec && te->when_ms < nearest->when_ms)) {
            nearest = te;
        }
        te = te->next;
    }
    return nearest;
}

/********************************** event process ***************************/
/**
 * 这个函数干了三件事：
 * 1. 判断每个 file event 的状态，添加到不同的集合中
 * 2. 找到最大的fd
 * 3. 返回 file event 的个数
 */
static int distinguishFileEventAndGetMaxFD(aeEventLoop *eventLoop, fd_set *rfds, fd_set *wfds, fd_set *efds, int *maxfd) {
    int numfd = 0;
    aeFileEvent *fe = eventLoop->fileEventHead;
    while (fe != NULL) {
        if (isReadable(fe->mask)) {
            FD_SET(fe->fd, rfds);
        }
        if (isWritable(fe->mask)) {
            FD_SET(fe->fd, wfds);
        }
        if (isException(fe->mask)) {
            FD_SET(fe->fd, efds);
        }
        if (*maxfd < fe->fd) {
            *maxfd = fe->fd;
        }
        numfd++;
        fe = fe->next;
    }

    return numfd;
}

/**
 * 计算 select 等待时间
 * @param tvp select timeout，由本函数填充
 * @return 如果找到了最近的 time event, 等待最近的 time event; 否则如果指定了 AE_DONT_WAIT 则填充0， 否则返回NULL, 表示无限期等待
 */
static struct timeval *fillSelectTimeout(aeEventLoop *eventLoop, int flags, struct timeval *tvp) {
    aeTimeEvent *shortest = NULL;
    // struct timeval tv, *tvp;
    if (isTimeEventSet(flags) && !isDontWaitSet(flags)) {
        shortest = aeSearchNearestTimer(eventLoop);
    }
    // 如果找到了最近的事件，tvp就是当前距离最近时间的剩余时间
    if (shortest != NULL) {
        long now_sec, now_ms;
        aeGetTime(&now_sec, &now_ms);
        tvp->tv_sec = shortest->when_sec - now_sec;
        if (shortest->when_ms < now_ms) {  // 不够减的情况
            tvp->tv_usec = ((shortest->when_ms + 1000) - now_ms) * 1000;
            tvp->tv_sec--;
        } else {
            tvp->tv_usec = (shortest->when_ms - now_ms) * 1000;
        }
    } else {
        // 如果设置了不用等，则填充0
        if (isDontWaitSet(flags)) {
            tvp->tv_sec = tvp->tv_usec = 0;
        } else {
            // 否则，无限等待
            tvp = NULL;
        }
    }
    return tvp;
}

/**
 * @return processed file event number
 */
static int processedFileEvent(aeEventLoop *eventLoop, fd_set *rfds, fd_set *wfds, fd_set *efds) {
    int processed = 0;
    aeFileEvent *fe = eventLoop->fileEventHead;
    while (fe != NULL) {
        int fd = (int) fe->fd;
        bool isReadable = isReadable(fe->mask) && FD_ISSET(fd, rfds);
        bool isWritable = isWritable(fe->mask) && FD_ISSET(fd, wfds);
        bool isException = isException(fe->mask) && FD_ISSET(fd, efds);
        if ( isReadable || isWritable || isException) {
            int mask = 0;
            if (isReadable) {
                mask |= AE_READABLE;
            }
            if (isWritable) {
                mask |= AE_WRITABLE;
            }
            if (isException) {
                mask |= AE_EXCEPTION;
            }
            fe->fileProc(eventLoop, fe->fd, fe->clientData, mask);
            processed++;
            /**
             * 经过一次处理后，file event list 可能已经发生了改变，
             * 所以需要清理一下 fd 的标记，同时同头开始
             */
            fe = eventLoop->fileEventHead;
            FD_CLR(fd, rfds);
            FD_CLR(fd, wfds);
            FD_CLR(fd, efds);
        } else {
            fe = fe->next;
        }
    }
    return processed;
}

static void processedTimeEvent(aeEventLoop *eventLoop) {
    aeTimeEvent *te = eventLoop->timeEventHead;
    // 为了避免死循环
    int maxId = eventLoop->timeEventNextId - 1;
    while (te != NULL) {
        if (te->id > maxId) {
            te = te->next;
            continue;
        }

        long now_sec, now_ms;
        aeGetTime(&now_sec, &now_ms);
        if (now_sec > te->when_sec || (now_sec == te->when_sec && now_ms >te->when_ms)) {
            long long id = te->id;
            // todo: timeProc 的返回值是什么意思，下一次触发的时间吗
            int retval = te->timeProc(eventLoop, id, te->clientData);
            /**
             * 经过一次处理后，time event list 可能已经改变了，我们需要从头开始.
             */
            if (retval != AE_NOMORE) {
                aeAddMillisecondsToNow(retval, &te->when_sec, &te->when_ms);
            } else {
                aeDeleteTimeEvent(eventLoop, id);
            }
            te = eventLoop->timeEventHead;
        } else {
            te = te->next;
        }
    }
}

/**
 * 
 * 
 * @return the number of events processed
 */
int aeProcessEvents(aeEventLoop *eventLoop, int flags) {
    AE_NOTUSED(flags);
    if (!isFileEventSet(flags) && !isTimeEventSet(flags)) {
        return 0;
    }

    fd_set rfds, wfds, efds;
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    FD_ZERO(&efds);

    int maxfd = 0;
    int numfd = 0;
    if (isFileEventSet(flags)) {
        numfd = distinguishFileEventAndGetMaxFD(eventLoop, &rfds, &wfds, &efds, &maxfd);
    }

    int processed = 0;
    /**
     * Note that we want call select() even if there are no file events to process 
     * as long as we wanto to process time events, in order to sleep until the next
     * time event is ready to fire.
     */
    if (numfd != 0 || isTimeEventSet(flags) && !isDontWaitSet(flags)) {
        struct timeval tv;
        struct timeval *tvp = fillSelectTimeout(eventLoop, flags, &tv);
        /**
         * select 的最后一个参数是 timeval，表示 select 在返回之前阻塞的时间:
         *  - timeval 的 tv_sec 和 tv_usec 都是0，表示立即返回
         *  - timeval 是 NULL 的话表示无限期等待
         */
        int retval = select(maxfd+1, &rfds, &wfds, &efds, tvp);
        if (retval > 0) {
            processed = processedFileEvent(eventLoop, &rfds, &wfds, &efds);
        }
    }

    if (isTimeEventSet(flags)) {
        processedTimeEvent(eventLoop);
    }

    return processed;
}

int aeWait(int fd, int mask, long long milliseconds) {
    struct timeval tv;
    tv.tv_sec = milliseconds / 1000;
    tv.tv_usec = (milliseconds % 1000) * 1000;
    fd_set rfds, wfds, efds;
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    FD_ZERO(&efds);

    if (isReadable(mask)) FD_SET(fd, &rfds);
    if (isWritable(mask)) FD_SET(fd, &wfds);
    if (isException(mask)) FD_SET(fd, &efds);

    int retval = select(fd+1, &rfds, &wfds, &efds, &tv);
    if (retval > 0) {
        int remask = 0;
        if (FD_ISSET(fd, &rfds)) remask |= AE_READABLE;
        if (FD_ISSET(fd, &wfds)) remask |= AE_WRITABLE;
        if (FD_ISSET(fd, &efds)) remask |= AE_EXCEPTION;
        return remask;
    } else {
        return retval;
    }
}

void aeMain(aeEventLoop *eventLoop) {
    eventLoop->stop = false;
    while (!eventLoop->stop) {
        aeProcessEvents(eventLoop, AE_ALL_EVENTS);
    }
}

/************************ for test ************************/
static count = 0;
void aeFileProcString(aeEventLoop *eventLoop, int fd, void *clientData, int mask) {
    // do nothing
}

int aeTimeProcString(aeEventLoop *eventLoop, long long id, void *clientData) {
    char *data = (char *) clientData;
    printf("id: %lld, data: %s\n", id, data);
    count++;
    return count * sizeof(data);
}

void aeEventFinalizerProcString(struct aeEventLoop *eventLoop, void *clientData) {
    printf("Free client data.\n");
}

int main() {
    aeEventLoop *eventLoop = aeCreateEventLoop();
    aeCreateTimeEvent(eventLoop, 1800, aeTimeProcString, "FFFFFFFFFFF", aeEventFinalizerProcString);
    aeCreateTimeEvent(eventLoop, 2800, aeTimeProcString, "SSSSSSSSSSS", aeEventFinalizerProcString);
    aeCreateTimeEvent(eventLoop, 3800, aeTimeProcString, "TTTTTTTTTTT", aeEventFinalizerProcString);
    aeCreateTimeEvent(eventLoop, 4800, aeTimeProcString, "F2F2F2F2F2F", aeEventFinalizerProcString);
    aeMain(eventLoop);
}