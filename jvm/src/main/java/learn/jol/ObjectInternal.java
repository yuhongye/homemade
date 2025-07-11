package learn.jol;


import org.openjdk.jol.info.ClassLayout;
import org.openjdk.jol.vm.VM;

public class ObjectInternal {
  public static void main(String[] args) {
    System.out.println(VM.current().details());
    System.out.println(ClassLayout.parseClass(Object.class).toPrintable());
  }
}
