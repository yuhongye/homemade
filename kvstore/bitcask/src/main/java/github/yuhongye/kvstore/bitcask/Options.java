package github.yuhongye.kvstore.bitcask;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Getter;
import lombok.ToString;

/**
 * 配置类
 */
@AllArgsConstructor
@Builder
@Getter
@ToString
public class Options {
    private int numThread;
    private int itemBufSize;
    private int maxConns;
    private int port;
    private byte[] inter;

    int verbose;

}
