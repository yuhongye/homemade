package github.yuhongye.kvstore.leveldb.util;

import java.util.concurrent.atomic.AtomicInteger;

/**
 * Leveldb 实现的简单内存池，貌似在 Java 实现中无用武之地
 */
public class Arena {
  // 当前已经分配处的指针，分割分配和未分配区域
  private int allocPtr;
  private int allocBytesRemaining;

  private byte[][] vectors;

  private AtomicInteger memoryUsage;

}
