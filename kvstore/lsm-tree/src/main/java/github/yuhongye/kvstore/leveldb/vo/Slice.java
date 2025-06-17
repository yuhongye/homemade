package github.yuhongye.kvstore.leveldb.vo;

import github.yuhongye.kvstore.common.bytes.Binary;

import java.nio.charset.StandardCharsets;

public class Slice extends Binary {

  public Slice() {
    this(EMPTY);
  }

  public Slice(byte[] data) {
    super(data, true);
  }

  public Slice(String s) {
    super(s.getBytes(StandardCharsets.UTF_8), true);
  }

  public Slice(Slice other) {
    super(other.data, other.offset, other.size, true);
  }

  public void clear() {
    this.data = EMPTY;
    this.size = 0;
    this.offset = 0;
  }

  /**
   * 删除前 n 个字节
   * @param n
   */
  public void removePrefix(int n) {
    assert  n < size;
    this.offset += n;
    size -= n;
  }

  public String asString() {
    return new String(data, offset, size, StandardCharsets.UTF_8);
  }

  @Override
  public String toString() {
    return String.format("Slice(offset=%d, size=%d)", offset, size);
  }
}
