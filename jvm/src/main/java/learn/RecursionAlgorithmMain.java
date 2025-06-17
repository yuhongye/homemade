package learn;

import java.io.IOException;

public class RecursionAlgorithmMain {
  public static void main(String[] args) throws IOException {
    new Thread(() -> sigma(1)).start();
    System.in.read();
  }

  public static int sigma(int n) {
    System.out.println("current 'n' value is " + n);
    return n + sigma(n + 1);
  }
}
