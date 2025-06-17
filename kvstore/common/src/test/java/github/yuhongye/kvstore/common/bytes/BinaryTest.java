package github.yuhongye.kvstore.common.bytes;

import org.junit.Test;

import java.util.HashSet;
import java.util.Set;

import static junit.framework.TestCase.assertFalse;
import static junit.framework.TestCase.assertTrue;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;

public class BinaryTest {
    @Test
    public void testBacked() {
        byte[] b = {0, 1, 2, 3, 4, 5, 6, 7, 7};
        Binary origin = new Binary(b, true);
        b[0] = -1;
        assertEquals(-1, origin.get(0));

        Binary origin2 = new Binary(b, 3, 3, false);
        b[4] = -4;
        assertEquals(4, origin2.get(1));
    }

    @Test
    public void testHashCodeAndEquals() {
        byte[] b1 = {0, 1, 2, 3, 4, 5};
        byte[] b2 = {1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5};
        Binary bb1 = new Binary(b1, true);
        Binary bb2 = new Binary(b2, 5, 6, true);
        assertEquals(bb1, bb2);

        Binary bb22 = new Binary(b2, true);
        assertNotEquals(bb2, bb22);

        Set<Binary> set = new HashSet<>();
        set.add(bb1);
        assertTrue(set.contains(bb2));
        assertFalse(set.contains(bb22));
    }
}
