package github.yuhongye.jvm.meta;

import lombok.AllArgsConstructor;

@AllArgsConstructor
public enum JDKVersion {
    JAVA_1_0_2(45, 3, "Java 1.0.2"),
    JAVA_1_1(45, 3, "Java 1.1"),
    JAVA_1_4(48, 0, "Java 1.4"),
    JAVA_5(  49, 0, "Java 5"),
    JAVA_6(  50, 0, "Java 6"),
    JAVA_7(  51, 0, "Java 7"),
    JAVA_8(  52, 0, "Java 8"),
    JAVA_9(  53, 0, "Java 9"),
    ;

    private int major;
    private int minor;
    private String jdkName;

    @Override
    public String toString() {
        return jdkName;
    }

    public static JDKVersion getByMajor(int version) {
        for(JDKVersion o : values()) {
            if (o.major == version) {
                return o;
            }
        }
        throw new RuntimeException("No jdk majar version is " + version);
    }
}
