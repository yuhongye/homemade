package github.yuhongye.jvm.parser;

import lombok.AllArgsConstructor;

import java.util.Arrays;
import java.util.Objects;
import java.util.stream.Collectors;

@AllArgsConstructor
public enum MethodAcc {
    ACC_PUBLIC(        0x0001, "声明为public, 可以包外访问"),
    ACC_PRIVATE(       0x0002, "声明为private, 只能在定义该字段的类中访问"),
    ACC_PROTECTED(     0x0004, "声明为protected, 子类可以访问"),
    ACC_STATIC(        0x0008, "声明为static"),
    ACC_FINAL(         0x0010, "生命为final, 不允许有子类"),
    ACC_SYNCHRONIZED(         0x0020, "声明为synchronized, 对该方法的调用将包装在同步锁(monitor)里"),
    ACC_BRIDGE(               0x0040, "声明为bridge方法，由编译器产生(桥接方法是JDK 1.5引入泛型后为了跟之前的字节码兼容，由编译器自动生成的方法)"),
    ACC_VARARGS(              0x0080, "表示方法带有变成参数"),
    ACC_NATIVE(               0x0100, "声明为native，该方法不是用Java语言实现的"),
    ACC_ABSTRACT(             0x0400, "该方法没有实现代码"),
    ACC_STRICT(               0x0800, "声明为strictfp，使用FP-strict浮点模式"),
    ACC_SYNTHETIC(            0x1000, "该方法是由编译器生成，而不是由源代码编译出来的")
    ;

    private int mask;
    private String desc;

    public static String toString(int accessFlag) {
        return Arrays.stream(values())
                .filter(acc -> (acc.mask & accessFlag) != 0)
                .map(Objects::toString)
                .collect(Collectors.joining(", "));
    }
}
