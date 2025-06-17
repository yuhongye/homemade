package github.yuhongye.kvstore.bitcask.util;


import lombok.extern.slf4j.Slf4j;

@Slf4j
public class HTree {
    public static final int TREE_BUF_SIZE = 512;
    public static final int DATA_BLOCK_SIZE_SMALL = 1024;
    public static final int DATA_BLOCK_SIZE = 256;
    public static final long[] G_INDEX = {0, 1, 17, 273, 4369, 69905, 1118481, 17895697, 286331153, 4581298449L};

    public static class Item {
        private int pos;
        private int ver;
        private int hash;
        private int ksz;
        private byte[] key;
    }

    public static class Data {
        private int size;
        private int used;
        private int count;
        private Data next;
        private Item[] head;

        public Data(int size) {
            this.size = size;
            //todo
            this.used = 0;
        }
    }

    public static class Node {
        private boolean isNode = true;
        private boolean valid = true;
        private int depth = 4;
        private int flag = 9;
        private int hash;
        private int count;
        private Data data;
    }

    private int depth;
    private int pos;
    private int height;
    private int blockSize;
    private Node[] root;
    private Codec dc;

    private Object lock;

    private byte[] buf = new byte[TREE_BUF_SIZE];

    private int updateBucket;

    private HTree updatingTree;

    public HTree(int depth, int pos, boolean tmp) {
        this.depth = depth;
        this.pos = pos;
        this.height = 1;
        this.blockSize = tmp ? DATA_BLOCK_SIZE_SMALL : DATA_BLOCK_SIZE;
        this.updateBucket = -1;
        this.updatingTree = null;

        this.root = new Node[height];
        for (int i = 0; i < height; i++) {
            for (long j = G_INDEX[i]; j < G_INDEX[i + 1]; j++) {
                Node node = new Node();
                node.depth = i;
                root[(int) j] = node;
            }
        }

        this.dc = new Codec();
        lock = new Object();
    }

    private void clear(Node node) {
        Data data = new Data(64);
        node.data = data;
        node.isNode = false;
        node.valid = true;
        node.hash = 0;
        node.count = 0;
    }

    public void add(byte[] key, int pos, int hash, int ver) {

    }

    boolean checkBucket(byte[] key, int len) {
        int hash = Fnv1aHash.fnv1a(key);
        if (depth > 0 && hash >> ((8 - depth) * 4) != pos) {
            log.error("Key %s");
        }
    }

}
