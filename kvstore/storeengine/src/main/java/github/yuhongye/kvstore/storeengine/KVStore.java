package github.yuhongye.kvstore.storeengine;

/**
 * KV 存储接口
 */
public interface KVStore {
  /**
   * 保存key value
   * @param key 不能为空
   * @param value 不能为空
   */
  void put(byte[] key, byte[] value);

  /**
   * @param key
   * @return key 对应的 value, or null if key do not exist
   */
  byte[] get(byte[] key);

  /**
   * @param key
   * @return key 对应的 value, or null if key do not exist
   */
  byte[] del(byte[] key);

}
