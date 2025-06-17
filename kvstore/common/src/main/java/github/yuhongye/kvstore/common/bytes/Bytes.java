package github.yuhongye.kvstore.common.bytes;

import static java.lang.Integer.min;

public class Bytes {

  public static int compare(byte[] b1, byte[] b2) {
    return compare(b1, 0, b1.length, b2, 0, b2.length);
  }

  public static int compare(byte[] b1, int offset1, int len1, byte[] b2, int offset2, int len2) {
    int size = min(len1, len2);

    for (int i = 0; i < size; i++) {
      int a = b1[offset1 + i];
      int b = b2[offset2 + i];
      if (a != b) {
          return (a & 0xFF) - (b & 0xFF);
      }
    }
    return len1 - len2;
  }
}
