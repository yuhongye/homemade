package github.yuhongye.jvm.parser;

import lombok.AllArgsConstructor;
import lombok.Getter;

import java.util.HashMap;
import java.util.Map;

@AllArgsConstructor
@Getter
public enum ConstantTag {
    CONSTANT_UTF8_INFO(               1, 1, "Constant_Utf8_info", "Utf8"),
    CONSTANT_INTEGER_INFO(            3, 1, "Constant_Integer_info", "Integer"),
    CONSTANT_FLOAT_INFO(              4, 1, "Constant_Float_info", "Float"),
    CONSTANT_LONG_INFO(               5, 2, "Constant_Long_info", "Long"),
    CONSTANT_DOUBLE_INFO(             6, 2, "Constant_Double_info", "Double"),
    CONSTANT_CLASS_INFO(              7, 1,"Constant_Class_info", "Class"),
    CONSTANT_STRING_INFO(             8, 1, "Constant_String_info", "String"),
    CONSTANT_FIELDREF_INFO(           9, 1, "Constant_FieldRef_info", "Fieldref"),
    CONSTANT_METHODREF_INFO(         10, 1, "Constant_MethodRef_info", "Methodref"),
    CONSTANT_INTERFACEMETHODREF_INFO(11, 1, "Constant_InterfaceMethodRef_info", "InterfaceMethodref"),
    CONSTANT_NAMEANDTYPE_INFO(  12, 1, "Constant_NameAndType_info", "NameAndType"),
    CONSTANT_METHODHANDLE_INFO( 15, 1, "Constant_MethodHandle_info", "Methodhandle"),
    CONSTANT_METHODTYPE_INFO(   16, 1, "Constant_MethodType_info", "Methodtype"),
    CONSTANT_INVOKEDYNAMIC_INFO(18, 1, "Constant_InvokeDynamic_info", "InvokeDynamic"),
    ;

    private int tag;
    private int slotSize;
    private String type;
    private String shortName;

    private static final Map<Integer, ConstantTag> tag2Enum = new HashMap<>();
    static {
        for (ConstantTag tag : values()) {
            tag2Enum.put(tag.tag, tag);
        }
    }

    @Override
    public String toString() {
        return shortName;
    }

    public static ConstantTag getByTag(int tag) {
        return tag2Enum.get(tag);
    }
}
