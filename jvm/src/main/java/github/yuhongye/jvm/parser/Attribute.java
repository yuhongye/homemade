package github.yuhongye.jvm.parser;

import github.yuhongye.jvm.meta.JDKVersion;
import lombok.AllArgsConstructor;

import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;
import java.util.Objects;

import static github.yuhongye.jvm.parser.Attribute.Location.C;
import static github.yuhongye.jvm.parser.Attribute.Location.M;
import static github.yuhongye.jvm.parser.Attribute.Location.F;
import static github.yuhongye.jvm.parser.Attribute.Location.CFM;

@AllArgsConstructor
public enum Attribute {
    SOURCE_FILE           ("SourceFile",           C, JDKVersion.JAVA_1_0_2),
    INNER_CLASS           ("InnerClass",           C, JDKVersion.JAVA_1_1),
    ENCLOSING_METHOD      ("EnclosingMethod",      C, JDKVersion.JAVA_5),
    SOURCE_DEBUG_EXTENSION("SourceDebugExtension", C, JDKVersion.JAVA_5),
    BOOTSTRAP_METHODS     ("BootstrapMethods",     C, JDKVersion.JAVA_7),

    CONSTANT_VALUE("ConstantValue", F, JDKVersion.JAVA_1_0_2),

    CODE                                   ("Code",                                 M, JDKVersion.JAVA_1_0_2),
    EXCEPTIONS                             ("Exceptions",                           M, JDKVersion.JAVA_1_0_2),
    RUNTIME_VISIBLE_PARAMETER_ANNOTATIONS  ("RuntimeVisibleParameterAnnotations",   M, JDKVersion.JAVA_5),
    RUNTIME_INVISIBLE_PARAMETER_ANNOTATIONS("RuntimeInVisibleParameterAnnotations", M, JDKVersion.JAVA_5),
    ANNOTATION_DEFAULT                     ("AnnotationDefault",                    M, JDKVersion.JAVA_5),
    METHOD_PARAMETERS                      ("MethodParameters",                     M, JDKVersion.JAVA_8),

    SYNTHETIC                         ("Synthetic",                       CFM, JDKVersion.JAVA_1_1),
    DEPRECATED                        ("Deprecated",                      CFM, JDKVersion.JAVA_1_1),
    SIGNATURE                         ("Signature",                       CFM, JDKVersion.JAVA_5),
    RUNTIME_VISIBLE_ANNOTATIONS       ("RuntimeVisibleAnnotations",       CFM, JDKVersion.JAVA_5),
    RUNTIME_INVISIBLE_ANNOTATIONS     ("RuntimeInVisibleAnnotations",     CFM, JDKVersion.JAVA_5),
    RUNTIME_VISIBLE_TYPE_ANNOTATIONS  ("RuntimeVisibleTypeAnnotations",   CFM, JDKVersion.JAVA_8),
    RUNTIME_INVISIBLE_TYPE_ANNOTATIONS("RuntimeInVisibleTypeAnnotations", CFM, JDKVersion.JAVA_8),

    LINENUMBER_TABLE         ("LineNumberTable",        Location.CODE, JDKVersion.JAVA_1_0_2),
    LOCAL_VARIABLE_TABLE     ("LocalVariableTable",     Location.CODE, JDKVersion.JAVA_1_0_2),
    LOCAL_VARIABLE_TYPE_TABLE("LocalVariableTypeTable", Location.CODE, JDKVersion.JAVA_5),
    STACK_MAP_TABLE          ("StackMapTable",          Location.CODE, JDKVersion.JAVA_6),
    ;


    /**
     * 属性名字，非常重要，通过名字来判断是哪个属性
     */
    private String name;

    /**
     * 属性出现的位置
     */
    private Location location;

    /**
     * 首先出现在哪个版本的jdk重
     */
    private JDKVersion jdkVersion;

    private static Map<String, Attribute> name2Instance = new HashMap<>();

    static {
        Arrays.stream(values()).forEach(attr -> name2Instance.put(attr.name, attr));
    }

    public static Attribute getAttributeByName(String name) {
        return Objects.requireNonNull(name2Instance.get(name));
    }

    /**
     * Class文件中预定义的属性出现的位置
     */
    public enum Location {
        C, F, M, CFM, CODE;
    }
}
