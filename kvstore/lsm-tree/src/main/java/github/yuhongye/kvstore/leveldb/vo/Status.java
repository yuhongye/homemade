package github.yuhongye.kvstore.leveldb.vo;

import github.yuhongye.encoding.FixIntCodec;
import lombok.AllArgsConstructor;
import lombok.Getter;
import lombok.NoArgsConstructor;

import static java.nio.charset.StandardCharsets.UTF_8;

/**
 * Leveldb 将错误码和错误信息打包在一起, 它没有使用 string，而是通过一种编码的方式来实现:
 * 1) state == null 表示成功
 * 2) state != null 表示错误:
 *  a. state[0, 3]: length of message
 *  b. state[4]   : error code
 *  c. state[5..] : message
 *
 */
@AllArgsConstructor
@NoArgsConstructor
public class Status {

  static final int CODE_INDEX = 4;
  static final int MSG_INDEX = CODE_INDEX + 1;

  public static final Status OK = new Status();

  @AllArgsConstructor
  @Getter
  enum Code {
    K_OK(0, "OK"),
    K_NOT_FOUND(1, "NotFound: "),
    K_CORRUPTION(2, "Corruption: "),
    K_NOT_SUPPORTED(3, "Not implemented: "),
    K_INVALID_ARGUMENT(4, "Invalid argument: "),
    K_IO_ERROR(5, "IO error: ");

    private int code;
    private String comment;

    static Code getByCode(int code) {
      switch (code) {
        case 0: return K_OK;
        case 1: return K_NOT_FOUND;
        case 2: return K_CORRUPTION;
        case 3: return K_NOT_SUPPORTED;
        case 4: return K_INVALID_ARGUMENT;
        case 5: return K_IO_ERROR;
        default: throw new RuntimeException("Unknow State.Code: " + code);
      }
    }
  }

  private byte[] state;

  public Status(Code code, Slice msg, Slice msg2) {
    int len1 = msg.size();
    int len2 = msg.size();
    int size = len1 + (len2 > 0 ? 2 + len2 : 0);
    byte[] state = new byte[size];
    FixIntCodec.writeLE(size, state);
    state[CODE_INDEX] = (byte) (code.getCode() & 0xFF);
    System.arraycopy(msg, msg.offset(), state, MSG_INDEX, msg.size());
    if (len2 > 0) {
      state[MSG_INDEX + len1] = ':';
      state[MSG_INDEX + 1 + len1] = ' ';
      System.arraycopy(msg2, msg2.offset(), state, MSG_INDEX + 2 + len2, msg2.size());
    }
  }

  public boolean ok() {
    return this.state == null;
  }

  public boolean isNotFound() {
    return code() == Code.K_NOT_FOUND;
  }

  public boolean isCorruption() {
    return code() == Code.K_CORRUPTION;
  }

  public boolean isIOError() {
    return code() == Code.K_IO_ERROR;
  }

  public boolean isNotSupportedError() {
    return code() == Code.K_NOT_SUPPORTED;
  }

  public boolean isInvalidArgument() {
    return code() == Code.K_INVALID_ARGUMENT;
  }

  private Code code() {
    return state == null ? Code.K_OK : Code.getByCode(state[CODE_INDEX] & 0xFF);
  }

  @Override
  public String toString() {
    Code code = code();
    if (code == Code.K_OK) {
      return code.getComment();
    } else {
      return code.getComment() + new String(state, MSG_INDEX, state.length - MSG_INDEX, UTF_8);
    }
  }
}
