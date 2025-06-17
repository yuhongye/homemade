package learn.jol;

import org.openjdk.jol.info.ClassLayout;

/**
 * identity hashcode store in the mark word.
 * 一旦计算后就不会再改变了：不是对象地址
 * 1. 良好的分布性：使用PRNG计算
 * 2. 幂等性：保存在 mark word 中
 */
public class IdentityHashCode {
  public static void main(String[] args) {
    final A a = new A();
    ClassLayout layout = ClassLayout.parseInstance(a);

    System.out.println("**** Fresh object");
    System.out.println(layout.toPrintable());

    System.out.println("hashCode: " + Integer.toHexString(a.hashCode()));
    System.out.println();

    System.out.println("**** After identityHashCode()");
    System.out.println(layout.toPrintable());
  }

  static class A { }
}
