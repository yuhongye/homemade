package bytecode.handler;

import bytecode.type.ClassFile;

import java.nio.ByteBuffer;

/**
 * class结构中各项的解析器抽象接口，
 * 实际的每个解析器应该只负责完成class文件结构中某一项的解析工作
 */
public interface BaseByteCodeHandler {
    /**
     * 解释器的排序值, 根据 class 文件结构定义的顺序，魔数解析器排在最前面，紧接着是版本号解析器
     * @return
     */
    int order();

    /**
     * 读取 class 文件中的内容，并填充到 ClassFile 中
     * @param buf
     * @param classFile
     * @throws Exception
     */
    void read(ByteBuffer buf, ClassFile classFile) throws Exception;
}
