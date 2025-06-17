package github.yuhongye.encoding;

public final class FixIntCodec {
  public static void writeLE(int value, byte[] data) {
    writeLE(value, data, 0);
  }

  public static void writeLE(int value, byte[] data, int offset) {
    data[offset + 0] = (byte) value;
    data[offset + 1] = (byte) (value >> 8);
    data[offset + 2] = (byte) (value >> 16);
    data[offset + 3] = (byte) (value >> 24);
  }

  public static int readLE(byte[] data) {
    return readLE(data, 0);
  }

  public static int readLE(byte[] data, int offset) {
    int value = data[offset] & 0xFF;
    value |= (data[offset + 1] & 0xFF) << 8;
    value |= (data[offset + 2] & 0xFF) << 16;
    value |= (data[offset + 3] & 0xFF) << 24;
    return value;
  }

}
