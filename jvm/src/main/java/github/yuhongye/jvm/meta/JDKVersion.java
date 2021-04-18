package github.yuhongye.jvm.meta;

import lombok.AllArgsConstructor;

@AllArgsConstructor
public enum JDKVersion {
    JAVA_1_4(48, "Java 1.4"),
    JAVA_5(  49, "Java 5"),
    JAVA_6(  50, "Java 6"),
    JAVA_7(  51, "Java 7"),
    JAVA_8(  52, "Java 8"),
    JAVA_9(  53, "Java 9"),
    ;

    private int major;
    private String jdkName;

    @Override
    public String toString() {
        return jdkName;
    }

    public static JDKVersion getByVersion(int version) {
        for(JDKVersion o : values()) {
            if (o.major == version) {
                return o;
            }
        }
        throw new RuntimeException("No jdk majar version is " + version);
    }
}
