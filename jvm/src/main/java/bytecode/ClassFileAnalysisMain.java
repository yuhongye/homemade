package bytecode;

import bytecode.type.ClassFile;
import bytecode.util.ClassFileAnalysiser;
import lombok.extern.slf4j.Slf4j;

import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;

@Slf4j
public class ClassFileAnalysisMain {

    public static void main(String[] args) throws Exception {
        ByteBuffer codeBuf = readFile(args[0]);
        ClassFile classFile = ClassFileAnalysiser.analysis(codeBuf);
        log.info("{} file: {}", args[0], classFile);
    }

    public static ByteBuffer readFile(String classFilePath) throws IOException {
        FileChannel channel = new RandomAccessFile(classFilePath, "r").getChannel();
        return channel.map(FileChannel.MapMode.READ_ONLY, 0, channel.size());
    }
}
