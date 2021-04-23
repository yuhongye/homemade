package github.yuhongye.jvm.parser;


import com.google.common.base.Preconditions;
import github.yuhongye.jvm.meta.JDKVersion;
import lombok.extern.slf4j.Slf4j;

import java.io.DataInput;
import java.io.DataInputStream;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.EnumMap;
import java.util.Map;

import static github.yuhongye.jvm.parser.ConstantTag.CONSTANT_CLASS_INFO;
import static github.yuhongye.jvm.parser.ConstantTag.CONSTANT_DOUBLE_INFO;
import static github.yuhongye.jvm.parser.ConstantTag.CONSTANT_FIELDREF_INFO;
import static github.yuhongye.jvm.parser.ConstantTag.CONSTANT_FLOAT_INFO;
import static github.yuhongye.jvm.parser.ConstantTag.CONSTANT_INTEGER_INFO;
import static github.yuhongye.jvm.parser.ConstantTag.CONSTANT_INTERFACEMETHODREF_INFO;
import static github.yuhongye.jvm.parser.ConstantTag.CONSTANT_INVOKEDYNAMIC_INFO;
import static github.yuhongye.jvm.parser.ConstantTag.CONSTANT_LONG_INFO;
import static github.yuhongye.jvm.parser.ConstantTag.CONSTANT_METHODHANDLE_INFO;
import static github.yuhongye.jvm.parser.ConstantTag.CONSTANT_METHODREF_INFO;
import static github.yuhongye.jvm.parser.ConstantTag.CONSTANT_METHODTYPE_INFO;
import static github.yuhongye.jvm.parser.ConstantTag.CONSTANT_NAMEANDTYPE_INFO;
import static github.yuhongye.jvm.parser.ConstantTag.CONSTANT_STRING_INFO;
import static github.yuhongye.jvm.parser.ConstantTag.CONSTANT_UTF8_INFO;

/**
 * Java class 文件结构
 * class file {
 *     u4 magic;
 *
 *     u2 minor_version;
 *     u2 majar_version;
 *
 *     u2 constant_pool_count;
 *     cp_info constant_pool[constant_pool_count - 1]; // 索引从1-constant_pool_count-1, 0 属性保留索引
 *
 *     u2 access_flags;
 *
 *     u2 this_class;
 *     u2 super_class;
 *     u2 interface_count;
 *     u2 interfaces[interface_count];
 *
 *     u2 fields_count;
 *     field_info fields[fields_count];
 *
 *     u2 methods_count;
 *     method_info = methods[methods_count];
 *
 *     u2 attributes_count;
 *     attribute_info attributes[attributes_count];
 * }
 */
@Slf4j
public class ClassParser {
    public static final int MAGIC = 0xCAFEBABE;

    public static Map<ConstantTag, ReadConstant<DataInput, ConstVal>> constantTagParser = new EnumMap<>(ConstantTag.class);
    static {
        constantTagParser.put(CONSTANT_INTEGER_INFO,            in -> new ConstVal(CONSTANT_INTEGER_INFO, in.readInt()));
        constantTagParser.put(CONSTANT_LONG_INFO,               in -> new ConstVal(CONSTANT_LONG_INFO, in.readLong()));
        constantTagParser.put(CONSTANT_FLOAT_INFO,              in -> new ConstVal(CONSTANT_FLOAT_INFO, in.readFloat()));
        constantTagParser.put(CONSTANT_DOUBLE_INFO,             in -> new ConstVal(CONSTANT_DOUBLE_INFO, in.readDouble()));

        constantTagParser.put(CONSTANT_UTF8_INFO,               ClassParser::readUtf8);
        constantTagParser.put(CONSTANT_STRING_INFO,             in -> new ConstVal(CONSTANT_STRING_INFO, in.readUnsignedShort()));

        constantTagParser.put(CONSTANT_CLASS_INFO,              ClassParser::readClassInfo);

        constantTagParser.put(CONSTANT_NAMEANDTYPE_INFO,        ClassParser::readNameAndTypeInfo);

        constantTagParser.put(CONSTANT_FIELDREF_INFO,           in -> new ConstVal(CONSTANT_FIELDREF_INFO, readRefInfo(in)));
        constantTagParser.put(CONSTANT_METHODREF_INFO,          in -> new ConstVal(CONSTANT_METHODREF_INFO, readRefInfo(in)));
        constantTagParser.put(CONSTANT_INTERFACEMETHODREF_INFO, in -> new ConstVal(CONSTANT_INTERFACEMETHODREF_INFO, readRefInfo(in)));

        constantTagParser.put(CONSTANT_METHODTYPE_INFO,         ClassParser::readMethodType);
        constantTagParser.put(CONSTANT_METHODHANDLE_INFO,       ClassParser::readMethodHandle);
        constantTagParser.put(CONSTANT_INVOKEDYNAMIC_INFO,      ClassParser::readDynamic);
    }

    private ConstantPool constantPool;

    public void checkCheckMagic(DataInput in) throws IOException {
        int magic = in.readInt();
        log.info("Magic: 0x{}", Integer.toHexString(magic).toUpperCase());
        Preconditions.checkArgument(magic == MAGIC, "This is not valid class file format.");
    }

    public void checkVersion(DataInput in) throws IOException {
        int minor = in.readShort();
        int major = in.readShort();
        log.info("Major version: {}, minor version: {}", major, minor);
        log.info("JDK VERSION: {}", JDKVersion.getByVersion(major));
    }

    private int readConstantPoolCountAndInit(DataInput in) throws IOException {
        int count = in.readUnsignedShort();
        constantPool = new ConstantPool(count);
        log.info("Constant pool count: {}", count);
        return count;
    }

    public void processConstantPool(DataInput in) throws Exception {
        int count = readConstantPoolCountAndInit(in);
        int i = 1;
        while (i < count) {
            int tag = in.readUnsignedByte();
            ConstantTag ctag = ConstantTag.getByTag(tag);
            ConstVal value = constantTagParser.get(ctag).read(in);
            log.info("#{} Read constant pool {}[{}], value: {}", i, ctag, tag, value.toString());
            constantPool.add(value);
            i += ctag.getSlotSize();
        }
    }

    static ConstVal readUtf8(DataInput in) throws IOException {
        return new ConstVal(CONSTANT_UTF8_INFO, in.readUTF());
    }

    static ConstVal readClassInfo(DataInput in) throws IOException {
        return new ConstVal(CONSTANT_CLASS_INFO, in.readUnsignedShort());
    }

    static ConstVal readNameAndTypeInfo(DataInput in) throws IOException {
        short nameIndex = in.readShort();
        short descIndex = in.readShort();
        return new ConstVal(CONSTANT_NAMEANDTYPE_INFO, new ConstVal.NameAndTypeInfo(nameIndex, descIndex));
    }

    static ConstVal.RefInfo readRefInfo(DataInput in) throws IOException {
        short classIndex = in.readShort();
        short nameAndTypeIndex = in.readShort();
        return new ConstVal.RefInfo(classIndex, nameAndTypeIndex);
    }

    static ConstVal readMethodType(DataInput in) throws IOException {
        short descIndex = in.readShort();
        return new ConstVal(CONSTANT_METHODTYPE_INFO, new ConstVal.MethodType(descIndex));
    }

    static ConstVal readMethodHandle(DataInput in) throws IOException {
        byte referenceKind = in.readByte();
        short referenceIndex = in.readShort();
        return new ConstVal(CONSTANT_METHODHANDLE_INFO, new ConstVal.MethodHandleInfo(referenceKind, referenceIndex));
    }

    static ConstVal readDynamic(DataInput in) throws IOException {
        short bootstrapMethodAttrIndex = in.readShort();
        short nameAndTypeIndex = in.readShort();
        return new ConstVal(CONSTANT_INVOKEDYNAMIC_INFO,
                new ConstVal.DynamicInfo(bootstrapMethodAttrIndex, nameAndTypeIndex));
    }

    public void readAccessFlag(DataInput in) throws IOException {
        short acc = in.readShort();
        String accInfo = ClassAcc.toString(acc);
        log.info("Access flag: {}", accInfo);
    }

    public void classSuperClass(DataInput in) throws IOException {
        int clsIndex = in.readUnsignedShort();
        ConstVal cls = constantPool.get(clsIndex);
        int superIndex = in.readUnsignedShort();
        ConstVal superCls = constantPool.get(superIndex);
        log.info("This class: {}, super class: {}", constantPool.toString(cls), constantPool.toString(superCls));
    }

    public void readInterfaces(DataInput in) throws IOException {
        int count = in.readUnsignedShort();
        log.info("Interface count: {}", count);
        for (int i = 1; i <= count; i++) {
            int interfaceIndex = in.readUnsignedShort();
            ConstVal val = constantPool.get(interfaceIndex);
            log.info("The {}th interface: {}", i, constantPool.toString(val));
        }
    }

    public void readFields(DataInput in) throws IOException {
        int count = in.readUnsignedShort();
        log.info("Field count: {}", count);
        for (int i = 0; i < count; i++) {
            readField(in);
        }
    }

    /**
     * 字段结构：
     * field_info {
     *     u2 access_flags;
     *     u2 name_index;
     *     u2 descriptor_index;
     *     u2 attributes_count;
     *     attribute_info attributes[attributes_count]
     * }
     *
     * @param in
     * @throws IOException
     */
    private void readField(DataInput in) throws IOException {
        int acc = in.readUnsignedShort();
        log.info("Field access flag: {}, desc: {}", acc, FieldAcc.toString(acc));
        int nameIndex = in.readUnsignedShort();
        String field = constantPool.toString(nameIndex);
        log.info("Field name index: {}, field: {}", nameIndex, field);
        int descIndex = in.readUnsignedShort();
        log.info("Field descriptor index: {}, desc: {}", descIndex, constantPool.toString(descIndex));
        int attributesCount = in.readUnsignedShort();
        log.info("Field attributes count: {}", attributesCount);
        for (int i = 0; i < attributesCount; i++) {
            readAttribute(in);
        }
    }

    public void readMethods(DataInput in) throws IOException {
        int count = in.readUnsignedShort();
        log.info("Method count: {}", count);
        for (int i = 0; i < count; i++) {
            readMethod(in);
        }
    }

    /**
     * 所有方法(包括实例初始化方法)都有method_info结构定义:
     * method_info {
     *     u2 access_flags;
     *     u2 name_index;
     *     u2 descriptor_index;
     *     u2 attributes_count;
     *     attribute_info attributes[attributes_count];
     * }
     * @param in
     */
    private void readMethod(DataInput in) throws IOException {
        int acc = in.readUnsignedShort();
        log.info("Method access flag: {}, desc: {}", acc, MethodAcc.toString(acc));
        int nameIndex = in.readUnsignedShort();
        String field = constantPool.toString(nameIndex);
        log.info("Method name index: {}, field: {}", nameIndex, field);
        int descIndex = in.readUnsignedShort();
        log.info("Method descriptor index: {}, desc: {}", descIndex, constantPool.toString(descIndex));
        int attributesCount = in.readUnsignedShort();
        log.info("Method attributes count: {}", attributesCount);
        for (int i = 0; i < attributesCount; i++) {
            readAttribute(in);
        }
    }

    public void readClassAttributes(DataInput in) throws IOException {
        int count = in.readUnsignedShort();
        log.info("Class attributes count: {}", count);
        for (int i = 0; i < count; i++) {
            readAttribute(in);
        }
    }

    /**
     * 属性在ClassFile, field_info, method_info, Code_attribute中都有使用.
     * 所有结构的通用格式如下:
     * attribute_info {
     *     u2 attribute_name_index; // 必须得是Constant_Utf8_info
     *     u4 attribute_length;
     *     u1 info[attribute_length];
     * }
     *
     * Java 8的规范中定义了23中属性
     *
     * @param in
     */
    private void readAttribute(DataInput in) throws IOException {
        int nameIndex = in.readUnsignedShort();
        ConstVal name = constantPool.get(nameIndex);
        Preconditions.checkState(name.getTag() == CONSTANT_UTF8_INFO,
                "属性的attribute_name_index对应的常量必须是Constant_Utf8_info");
        int length = in.readInt();
        byte[] attribute = new byte[length];
        in.readFully(attribute);
        log.info("\tattribute name: {}, length: {}", name.toString(), length);
    }

    public static void main(String[] args) throws Exception {
        DataInputStream in = new DataInputStream(new FileInputStream("/Users/caoxiaoyong/Desktop/temp/HelloWorld.class"));
        ClassParser parser = new ClassParser();
        parser.checkCheckMagic(in);
        parser.checkVersion(in);
        parser.processConstantPool(in);
        log.info("{}", parser.constantPool.toString());
        parser.readAccessFlag(in);
        parser.classSuperClass(in);
        parser.readInterfaces(in);
        parser.readFields(in);
        parser.readMethods(in);
        parser.readClassAttributes(in);
        Preconditions.checkState(in.available() == 0);
    }
}
