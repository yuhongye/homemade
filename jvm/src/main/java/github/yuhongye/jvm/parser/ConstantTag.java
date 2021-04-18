package github.yuhongye.jvm.parser;

import lombok.AllArgsConstructor;

import java.util.HashMap;
import java.util.Map;

@AllArgsConstructor
public enum ConstantTag {
    CONSTANT_UTF8_INFO(               1, "Constant_Utf8_info"),
    CONSTANT_INTEGER_INFO(            3, "Constant_Integer_info"),
    CONSTANT_FLOAT_INFO(              4, "Constant_Float_info"),
    CONSTANT_LONG_INFO(               5, "Constant_Long_info"),
    CONSTANT_DOUBLE_INFO(             6, "Constant_Double_info"),
    CONSTANT_CLASS_INFO(              7, "Constant_Class_info"),
    CONSTANT_STRING_INFO(             8, "Constant_String_info"),
    CONSTANT_FIELDREF_INFO(           9, "Constant_FieldRef_info"),
    CONSTANT_METHODREF_INFO(         10, "Constant_MethodRef_info"),
    CONSTANT_INTERFACEMETHODREF_INFO(11, "Constant_InterfaceMethodRef_info"),
    CONSTANT_NAMEANDTYPE_INFO(  12, "Constant_NameAndType_info"),
    CONSTANT_METHODHANDLE_INFO( 15, "Constant_MethodHandle_info"),
    CONSTANT_METHODTYPE_INFO(   16, "Constant_MethodType_info"),
    CONSTANT_INVOKEDYNAMIC_INFO(18, "Constant_InvokeDynamic_info"),
    ;

    private int tag;
    private String type;

    private static final Map<Integer, ConstantTag> tag2Enum = new HashMap<>();
    static {
        for (ConstantTag tag : values()) {
            tag2Enum.put(tag.tag, tag);
        }
    }

    @Override
    public String toString() {
        return type;
    }

    public static ConstantTag getByTag(int tag) {
        return tag2Enum.get(tag);
    }
}
