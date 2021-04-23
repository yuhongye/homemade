package github.yuhongye.jvm.parser;

import lombok.AllArgsConstructor;

import java.util.Arrays;
import java.util.Objects;
import java.util.stream.Collectors;

@AllArgsConstructor
public enum ClassAcc {
    ACC_PUBLIC(    0x0001, "声明为public, 可以包外访问"),
    ACC_FINAL(     0x0010, "生命为final, 不允许有子类"),
    ACC_SUPER(     0x0020, "不再使用(JDK 1.0.2之前使用)"),
    ACC_INTERFACE( 0x0200, "该类文件定义的是接口而不是类"),
    ACC_ABSTRACT(  0x0400, "声明为abstract，不能被实例化"),
    ACC_SYNTHETIC( 0x1000, "表明该class文件并非由Java源代码生成"),
    ACC_ANNOTATION(0x2000, "标识注解类型"),
    ACC_ENUM(      0x4000, "标识枚举类型")
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
