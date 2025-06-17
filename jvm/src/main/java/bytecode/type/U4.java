package bytecode.type;

public class U4 {
    private int value;

    public U4(byte b1, byte b2, byte b3, byte b4) {
        value = (b1 & 0xFF) << 24 | (b2 & 0xFF) << 16 | (b3 & 0xFF) << 8 | (b4 & 0xFF);
    }

    public int toInt() {
        return value;
    }

    public String toHexString() {
        return Integer.toHexString(value);
    }
}
