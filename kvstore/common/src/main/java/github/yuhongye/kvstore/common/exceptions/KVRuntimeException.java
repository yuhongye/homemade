package github.yuhongye.kvstore.common.exceptions;

public class KVRuntimeException extends RuntimeException {
  public KVRuntimeException() { }

  public KVRuntimeException(String message) {
    super(message);
  }

  public KVRuntimeException(String message, Throwable cause) {
    super(message, cause);
  }

  public KVRuntimeException(Throwable cause) {
    super(cause);
  }

  public KVRuntimeException(String message, Throwable cause, boolean enableSuppression, boolean writableStackTrace) {
    super(message, cause, enableSuppression, writableStackTrace);
  }
}
