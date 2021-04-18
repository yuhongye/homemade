package github.yuhongye.jvm.parser;

@FunctionalInterface
public interface ReadConstant<T, R> {
    R read(T t) throws Exception;
}
