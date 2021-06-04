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
    private ConstTag tag;
    private Object val;

    public int asInt() {
        Preconditions.checkState(tag == ConstTag.CONSTANT_INTEGER_INFO);
        return ((Integer) val).intValue();
    }

    public long asLong() {
        Preconditions.checkState(tag == ConstTag.CONSTANT_INTEGER_INFO);
        return ((Long) val).longValue();
    }

    public double asFloat() {
        Preconditions.checkState(tag == ConstTag.CONSTANT_FLOAT_INFO);
        return ((Float) val).floatValue();
    }

    public double asDouble() {
        Preconditions.checkState(tag == ConstTag.CONSTANT_DOUBLE_INFO);
        return ((Double) val).doubleValue();
    }

    public String asString() {
        Preconditions.checkState(tag == ConstTag.CONSTANT_STRING_INFO || tag == ConstTag.CONSTANT_UTF8_INFO);
        return val.toString();
    }

    @Override
    public String toString() {
        switch (tag) {
            case CONSTANT_LONG_INFO: return val.toString() + "L";
            case CONSTANT_FLOAT_INFO: return val.toString() + "F";
            case CONSTANT_DOUBLE_INFO: return val.toString() + "D";
            case CONSTANT_STRING_INFO: return "#" + val.toString();
            case CONSTANT_CLASS_INFO: return "#" + val.toString();
            default:
                return val.toString();
        }
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

        @Override
        public String toString() {
            return "#" + getNameIndex() + ".#" + getDescriptorIndex();
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

        @Override
        public String toString() {
            return "#" + getClassIndex() + ".#" + getNameAndTypeIndex();
        }
    }

    /**
     * Java 7 开始，为了更好的支持动态语言调用，新增了3中常量类型:
     *  - CONSTANT_MethodType_info
     *  - CONSTANT_MethodHandle_info
     *  - CONSTANT_InvokeDynamic_info
     */
    @AllArgsConstructor
    static class MethodHandleInfo {
        /**
         * 方法句柄的类型，值范围必须是1-9
         */
        byte referenceKind;

        /**
         * 1. 如果{@link #referenceKind} 的值是1, 2, 3, 4，那么reference_index对应的索引处成员必须是Constant_Fieldref_info
         * 2. 如果{@link #referenceKind} 的值是5, 8, 那么reference_index对应的索引处成员必须是Constant_Methodref_info
         * 3. 如果{@link #referenceKind} 的值是6, 7, 那么reference_index对应的索引处成员必须是Constant_Methodref_info 或者
         *    是Constant_InterfaceMethodref_info(文件版本大于等于52.0, 如果小于52.0 只能是Constant_Methodref_info)
         * 4. 如果{@link #referenceKind} 的值是9, 那么reference_index对应的索引处成员必须是Constant_InterfaceMethodref_info
         *
         * 1, 2, 3, 4 是针对字段的；
         * 5, 6, 7, 8, 9 是针对方法调用的。其中8(Ref_newInvokeSpecial)，那么Constant_Methodref_info结构所表示的方法其名称必须是<init>;
         * 5, 6, 7, 9 那么由Constant_Methodref_info 或者 Constant_InterfaceMethodref_info 结构锁表示的方法名称不能为<init>或者<cinit>
         */
        short referenceIndex;

        public int getReferenceKind() {
            return referenceKind & 0xFF;
        }

        public int getReferenceIndex() {
            return referenceIndex & 0xFFFF;
        }

        @Override
        public String toString() {
            return "#" + getReferenceKind() + ".#" + getReferenceIndex();
        }
    }

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

        @Override
        public String toString() {
            return "#" + getBootstrapMethodAttrIndex() + ".#" + getNameAndTypeIndex();
        }
    }

    /**
     * 方法的描述符
     */
    @AllArgsConstructor
    static class MethodType {
        /**
         * 必须对应常量池的Constant_Utf8_info
         */
        short descriptorIndex;

        int getDescriptorIndex() {
            return descriptorIndex & 0xFFFF;
        }

        @Override
        public String toString() {
            return "#" + getDescriptorIndex();
        }
    }
}
