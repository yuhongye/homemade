package github.yuhongye.jvm.parser;

/**
 * 常量池, 提供按索引访问的能力: 从下表1开始
 */
public class ConstantPool {
    /**
     * index: 从1开始，0闲置不用
     */
    private ConstVal[] vals;
    private int size = 1;

    public ConstantPool(int constantPoolCount) {
        vals = new ConstVal[constantPoolCount];
    }

    public void add(ConstVal val) {
        vals[size] = val;
        size += val.getTag().getSlotSize();
    }

    /**
     * @param index
     * @return 常量池的第 index 个常量
     */
    public ConstVal get(int index) {
        return vals[index];
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("Constant pool: \n");
        int i = 1;
        while (i < size) {
            ConstVal value = get(i);
            const2String(value, i, sb);
            i += value.getTag().getSlotSize();
        }
        return sb.toString();
    }

    private void const2String(ConstVal value, int index, StringBuilder sb) {
        sb.append("#").append(index)
                .append(" = ")
                .append(value.getTag())
                .append("\t").append(value)
                .append("\n");
    }

    public String toString(int index) {
        return toString(get(index));
    }

    public String toString(ConstVal value) {
        if (value.getVal() instanceof Integer && value.getTag() != ConstantTag.CONSTANT_INTEGER_INFO) {
            return get(((Integer) value.getVal()).intValue()).toString();
        } else {
            return value.toString();
        }
    }




}
