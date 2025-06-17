package learn.jol;

import org.openjdk.jol.info.ClassLayout;
import org.openjdk.jol.vm.VM;

import java.io.PrintWriter;

/**
 * 当对象经历了若干次 GC 后会被提升到老年代，观察object age(store in mark word) 的变化
 * vm: -Xmx512m  -XX:+UseG1GC -XX:MaxTenuringThreshold=13
 * 可以看到对象被移动了13次，注: G1 中晋升老年代的最大配置是15
 */
public class Promotion {
  static volatile Object sink;

  public static void main(String[] args) {
    System.out.println(VM.current().details());

    PrintWriter pw = new PrintWriter(System.out, true);

    Object o = new Object();
    ClassLayout layout = ClassLayout.parseInstance(o);

    long lastAddr = VM.current().addressOf(o);
    pw.printf("*** Fresh object is at %x%n", lastAddr);
    System.out.println(layout.toPrintable());

    int moves = 0;
    for (int i = 0; i < 10000000; i++) {
      long cur = VM.current().addressOf(o);
      if (cur != lastAddr) {
        moves++;
        pw.printf("*** Move %2d, object is at %x%n", moves, cur);
        System.out.println(layout.toPrintable());
        lastAddr = cur;
      }

      // make garbage
      for (int c = 0;  c < 100000; c++) {
        sink = new Object();
      }

    }

    long findAddr = VM.current().addressOf(o);
    pw.printf("*** Final object is at %x%n", findAddr);
    System.out.println(layout.toPrintable());

    pw.close();
  }
}
