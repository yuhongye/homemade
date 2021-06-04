package github.yuhongye.jvm.parser;

import com.google.common.base.Preconditions;
import lombok.extern.slf4j.Slf4j;

import java.io.DataInput;
import java.io.IOException;

@Slf4j
public class AttributeParser {
    public static final int CONSTANT_VALUE_LENGTH = 2;

    private ConstPool constPool;

    /**
     * Attribute结构的第一个字段都是attribute_name_index，它必须是对常量池的一个有效索引,
     * 并且该常量必须是Constant_Utf8_info结构
     * @param attributeNameIndex
     */
    public Attribute getAttributeType(int attributeNameIndex) {
        ConstVal value = constPool.get(attributeNameIndex);
        Preconditions.checkState(value.getTag() == ConstTag.CONSTANT_UTF8_INFO, "属性名必须是Constant_Utf8_info类型");
        String attributeName = value.asString();
        Attribute attribute = Attribute.getAttributeByName(attributeName);
        log.info("Attribute name: {}, attribute enum: {}", attributeName, attribute);
        return attribute;
    }

    /**
     * ConstantValue_attribute {
     *     u2 attribute_name_index;
     *     u4 attribute_length; // 固定是2
     *     u2 constantvalue_index;
     * }
     * constantvalue_index对应常量池的一个有效索引，该常量的成员给出了该属性表表示的常量值，这个常量池的类型必须使用当前字段:
     * long  :  {@link ConstTag#CONSTANT_LONG_INFO}
     * float : {@link ConstTag#CONSTANT_FLOAT_INFO}
     * double: {@link ConstTag#CONSTANT_DOUBLE_INFO}
     * int, short, char, byte, boolean: {@link ConstTag#CONSTANT_INTEGER_INFO}
     * String: {@link ConstTag#CONSTANT_STRING_INFO}
     *
     * Notice: 只有静态字段的ConstantValue需要解析，非静态字段的该属性需要扔掉
     * @param in
     */
    private void constantValue(DataInput in) throws IOException {
        // todo: 检查该字段是否是静态字段
        int length = in.readInt();
        Preconditions.checkState(length == CONSTANT_VALUE_LENGTH,
                "ConstantValue.length must be " + CONSTANT_VALUE_LENGTH + ", but its " + length);
        int constValueIndex = in.readUnsignedShort();
        ConstVal value = constPool.get(constValueIndex);
        log.info("ConstantValue: {}", value);
    }

}
