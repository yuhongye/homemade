package bytecode.util;

import bytecode.handler.BaseByteCodeHandler;
import bytecode.type.ClassFile;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

public class ClassFileAnalysiser {
    private static final List<BaseByteCodeHandler> HANDLERS = new ArrayList<>();

    /**
     * 添加各项的解析器
     */
    static {

        // 按照顺序来解析, 所以需要先排序
        HANDLERS.sort(Comparator.comparing(BaseByteCodeHandler::order));
    }

    /**
     * 将 class 文件中的内容生成 ClassFile 对象
     * @param codeBuf class 文件的内容读取到 buffer 中
     * @return
     * @throws Exception
     */
    public static ClassFile analysis(ByteBuffer codeBuf) throws Exception {
        ClassFile classFile = new ClassFile();
        codeBuf.position(0);
        for (BaseByteCodeHandler handler : HANDLERS) {
            handler.read(codeBuf, classFile);
        }
        return classFile;
    }
}
