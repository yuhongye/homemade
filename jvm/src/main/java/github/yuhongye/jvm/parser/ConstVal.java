package github.yuhongye.jvm.parser;

import com.google.common.base.Preconditions;
import lombok.AllArgsConstructor;
import lombok.Getter;

/**
 * 常量
 */
@AllArgsConstructor
@Getter
public class ConstVal {
    /**
     * 为了简单起见，通过tag来标明是什么类型, 不再创建子类了
     */
    private ConstantTag tag;
    private Object val;

    public int asInt() {
        Preconditions.checkState(tag == ConstantTag.CONSTANT_INTEGER_INFO);
        return ((Integer) val).intValue();
    }

    public long asLong() {
        Preconditions.checkState(tag == ConstantTag.CONSTANT_INTEGER_INFO);
        return ((Long) val).longValue();
    }

    public double asFloat() {
        Preconditions.checkState(tag == ConstantTag.CONSTANT_FLOAT_INFO);
        return ((Float) val).floatValue();
    }

    public double asDouble() {
        Preconditions.checkState(tag == ConstantTag.CONSTANT_DOUBLE_INFO);
        return ((Double) val).doubleValue();
    }

    public String asString() {
        Preconditions.checkState(tag == ConstantTag.CONSTANT_STRING_INFO || tag == ConstantTag.CONSTANT_UTF8_INFO);
        return val.toString();
    }

    @Override
    public String toString() {
        return val.toString();
    }

    @AllArgsConstructor
    static class NameAndTypeInfo {
        // 字段或者方法名称在常量池的下标
        short nameIndex;

        // 字段或方法的描述在常量池的下标
        short descriptorIndex;

        int getNameIndex() {
            return nameIndex & 0xFFFF;
        }

        int getDescriptorIndex() {
            return descriptorIndex & 0xFFFF;
        }
    }

    /**
     * FieldRef_Info, MethodRef_Info, InterfaceMethodRef_info
     */
    @AllArgsConstructor
    static class RefInfo {
        // 所属的类信息在常量池中的下标
        short classIndex;

        /** 指向一个 {@link NameAndTypeInfo} 的下标 表示方法名、参数和返回值类型 */
        short nameAndTypeIndex;

        int getClassIndex() {
            return classIndex & 0xFFFF;
        }

        int getNameAndTypeIndex() {
            return nameAndTypeIndex & 0xFFFF;
        }
    }

    /**
     * Java 7 开始，为了更好的支持动态语言调用，新增了3中常量类型:
     *  - CONSTANT_MethodType_info
     *  - CONSTANT_MethodHandle_info
     *  - CONSTANT_InvokeDynamic_info
     */
    @AllArgsConstructor
    static class DynamicInfo {
        /**
         * 执行导引方法表 bootstrap_methods[] 数组的索引
         */
        short bootstrapMethodAttrIndex;

        /**
         * 指向{@link NameAndTypeInfo}的索引，表示方法描述符
         */
        short nameAndTypeIndex;

        public int getBootstrapMethodAttrIndex() {
            return bootstrapMethodAttrIndex & 0xFFFF;
        }

        public int getNameAndTypeIndex() {
            return nameAndTypeIndex & 0xFFFF;
        }
    }
}
