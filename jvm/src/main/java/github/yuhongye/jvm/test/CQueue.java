package github.yuhongye.jvm.test;

import java.util.Arrays;
import java.util.stream.Collectors;

public class CQueue<E> {
    public static final int MAX_SIZE = 1024;
    public static final Object NULL = new Object();

    public int capacity;
    protected int next = 0;
    Object[] data;
    private boolean autoResize = false;

    public CQueue(int capacity) {
        if (capacity > MAX_SIZE) {
            throw new IllegalArgumentException("capacity is too big.");
        }
        this.capacity = capacity;
        data = new Object[capacity];
    }

    public void add(E e) {
        if ((next == capacity && !autoResize) || capacity == MAX_SIZE ) {
            throw new ArrayIndexOutOfBoundsException("Queue is full.");
        }

        if (next == capacity) {
            resize();
        }
        data[next++] = e;
    }

    private void resize() {
        int newSize = Integer.max(MAX_SIZE, capacity * 2);
        Object[] newData = new Object[newSize];
        System.arraycopy(data, 0, newData, 0, next);
        this.data = newData;
    }

    public Object get(int index) {
        if (index < 0 && index >= next) {
            throw new ArrayIndexOutOfBoundsException(index + " is not in [0, " + next + "]");
        }
        return data[index];
    }

    @Override
    public String toString() {
        return Arrays.stream(data).limit(next).map(Object::toString).collect(Collectors.joining(", ", "[", "]"));
    }

    public static void main(String[] args) {
        CQueue<String> queue = new CQueue<>(16);
        for (int i = 0; i < 16; i++) {
            queue.add("E" + i);
        }

        for (int i = 0; i < 16; i++) {
            System.out.println(queue.get(i));
        }
    }
}
