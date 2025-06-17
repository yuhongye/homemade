package github.yuhongye.kvstore.common.bytes;

import java.io.Serializable;

/**
 * 给 byte[] 增加 equals 和 hashcode
 */
public class Binary implements Comparable<Binary>, Serializable {
    public static final byte[] EMPTY = new byte[0];

    protected byte[] data;
    protected int offset;
    protected int size;

    /** isBacked指明是直接保存bytes，还是拷贝一份 */
    public Binary(byte[] bytes, boolean isBacked) {
        this(bytes, 0, bytes.length, isBacked);
    }

    public Binary(byte[] bytes, int offset, int len, boolean isBacked) {
        if (isBacked) {
            this.data = bytes;
            this.offset = offset;
        } else {
            this.data = new byte[len];
            System.arraycopy(bytes, offset, this.data, 0, len);
            this.offset = 0;
        }
        this.size = len;
    }

    public static Binary wrapper(byte[] bytes) {
        return new Binary(bytes, true);
    }

    /**
     * @param i
     * @return
     * @throws ArrayIndexOutOfBoundsException
     */
    public byte get(int i) {
        assert i < size;
        return data[offset + i];
    }

    public byte[] data() {
        return data;
    }

    public int offset() {
        return offset;
    }

    public int size() {
        return size;
    }

    public boolean isEmpty() {
        return size == 0;
    }

    public boolean isNotEmpty() {
      return !isEmpty();
    }

    @Override
    public int hashCode() {
        int result = 1;
        for (int i = offset; i < offset + size; i++) {
            result = 31 * result + data[i];
        }
        return result;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (obj == null || getClass() != obj.getClass()) {
            return false;
        }

        Binary o = (Binary) obj;

        if (size != o.size) {
            return false;
        }
        int i = 0;
        for (i = 0; i < size && data[i + offset] == o.data[i + o.offset]; i++) { }
        return i == size;
    }

    /**
     * 逐个判断元素的大小，如果全部相等，则比较长度
     * @param o
     * @return
     */
    @Override
    public int compareTo(Binary o) {
        return Bytes.compare(this.data, this.offset, this.size, o.data, o.offset, o.size);
    }

    @Override
    public String toString() {
      return String.format("Binary(offset=%d,size=%d)", offset, size);
    }
}
