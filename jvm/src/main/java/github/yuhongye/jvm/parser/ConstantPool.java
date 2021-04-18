package github.yuhongye.jvm.parser;

/**
 * 常量池, 提供按索引访问的能力: 从下表1开始
 */
public class ConstantPool {
    /**
     * index: 从1开始，0闲置不用
     */
    private ConstVal[] vals;

    public ConstantPool(int constantPoolCount) {
        vals = new ConstVal[constantPoolCount];
    }

    public void add(ConstVal val) {

    }

    /**
     * @param index
     * @return 常量池的第 index 个常量
     */
    public ConstVal get(int index) {
        return vals[index];
    }

}
