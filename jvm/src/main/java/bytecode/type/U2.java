package bytecode.type;

public class U2 {
    private int value;

    public U2(byte b1, byte b2) {
        value = (b1 & 0xFF) << 8 | (b2 & 0xFF);
    }

    public int toInt() {
        return value;
    }

    public String toHexString() {
        return Integer.toHexString(value);
    }
}
