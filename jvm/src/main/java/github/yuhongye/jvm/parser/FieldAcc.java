package github.yuhongye.jvm.parser;

import lombok.AllArgsConstructor;

import java.util.Arrays;
import java.util.Objects;
import java.util.stream.Collectors;

@AllArgsConstructor
public enum FieldAcc {
    ACC_PUBLIC(        0x0001, "声明为public, 可以包外访问"),
    ACC_PRIVATE(       0x0002, "声明为private, 只能在定义该字段的类中访问"),
    ACC_PROTECTED(     0x0004, "声明为protected, 子类可以访问"),
    ACC_STATIC(        0x0008, "声明为static"),
    ACC_FINAL(         0x0010, "生命为final, 不允许有子类"),
    ACC_VOLATILE(      0x0040, "声明为volatile，被标识的字段无法缓存"),
    ACC_TRANSIENT(     0x0080, "被标识的字段不会为持久化对象管理器所写入或读取"),
    ACC_SYNTHETIC(     0x1000, "该字段由编译器生成，而没有写在源代码中"),
    ACC_ENUM(          0x4000, "该字段声明为某个枚举类型的成员")
    ;

    private int mask;
    private String desc;

    public boolean is(int accessFlag) {
        return (mask & accessFlag) != 0;
    }

    public static String toString(int accessFlag) {
        return Arrays.stream(values())
                .filter(acc -> (acc.mask & accessFlag) != 0)
                .map(Objects::toString)
                .collect(Collectors.joining(", "));
    }
}
