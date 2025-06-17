package github.yuhongye.kvstore.bitcask.util;

public final class Fnv1aHash {
    public static final int FNV_32_PRIME = 0x01000193;
    public static final int  FNV_32_INIT = 0x811c9dc5;

    public static int fnv1a(byte[] key) {
        int hash = FNV_32_INIT;
        for (int i = 0; i < key.length; i++) {
            hash ^= (key[i] & 0xFF);
            hash *= FNV_32_PRIME;
        }
        return hash;
    }
}
