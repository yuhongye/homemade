package github.yuhongye.encoding;

import org.junit.Test;

import static org.junit.Assert.assertEquals;

public class FixIntCodecTest {
  @Test
  public void testLE() {
    testLE(-300, 300);
    testLE(Integer.MAX_VALUE - 1024, Integer.MAX_VALUE);
    testLE(Integer.MIN_VALUE, Integer.MIN_VALUE + 1024);
  }

  private void testLE(int start, int end) {
    byte[] data = new byte[8];
    for (int i = start; i >= start && i <= end; i++) {
      int offset = Math.abs(i) % 4;
      FixIntCodec.writeLE(i, data, offset);
      assertEquals(i, FixIntCodec.readLE(data, offset));
    }
  }
}
