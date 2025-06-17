package github.yuhongye.kvstore.common.bytes;

import org.junit.Test;

import static org.junit.Assert.assertTrue;

public class BytesTest {
  @Test
  public void testWhole() {
    byte[] b1 = new byte[] {1, 2, 3, 4, 5, 6, 7, 8};
    byte[] b2 = new byte[] {1, 2, 3, 4, 5, 6, 8};
    assertTrue(Bytes.compare(b1, b2) < 0);

    b1 = new byte[] {1, 2, 3, 4, 5, 6, 7, 8};
    b2 = new byte[] {2, 2, 3, 4, 5, 6, 7, 8};
    assertTrue(Bytes.compare(b1, b2) < 0);

    b1 = new byte[] {-1, 2, 3, 4, 5, 6, 7, 8};
    b2 = new byte[] {-128, 2, 3, 4, 5, 6, 7, 8};
    assertTrue(Bytes.compare(b1, b2) > 0);

    b1 = new byte[] {-1, 2, 3, 4, 5, 6, 7, 8};
    b2 = new byte[] {-1, 2, 3, 4, 5, 6, 7, 8};
    assertTrue(Bytes.compare(b1, b2) == 0);
  }

  @Test
  public void testOffset() {
    byte[] b1 = new byte[] {1, 2, 3, 4, 5, 6, 7};
    byte[] b2 = new byte[] {1, 2, 3, 4, 5, 6, 7, 8};

    assertTrue(Bytes.compare(b1, 0, 7, b2, 0, 7) == 0);
    assertTrue(Bytes.compare(b1, 0, 7, b2, 0, 6) > 0);
    assertTrue(Bytes.compare(b1, 0, 7, b2, 0, 8) < 0);
  }

}
