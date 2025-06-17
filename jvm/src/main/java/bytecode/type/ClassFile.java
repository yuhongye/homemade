package bytecode.type;

/**
 * java class file layout
 */
public class ClassFile {
    // 魔数
    private U4 magic;

    // 副版本号和朱版本号
    private U2 minor_version;
    private U2 magor_version;

    /**
     * 常量池数据及常量池
     */
    private U2 constant_pool_count;
    private CpInfo[] constant_pool;

    // 访问标志
    private U2 access_flags;

    // 类索引
    private U2 this_class;

    /**
     * 父类索引
     */
    private U2 super_class;

    /**
     * 接口总数及接口索引
     */
    private U2 interface_count;
    private U2[] interfaces;

    /**
     * 字段数及字段表
     */
    private U2 fields_count;
    private FieldInfo[] fields;

    /**
     * 方法数量和方法表
     */
    private U2 methods_count;
    private MethodInfo[] methods;

    /**
     * 属性数及属性表
     */
    private U2 attributes_count;
    private AttributeInfo[] attributes;
}
