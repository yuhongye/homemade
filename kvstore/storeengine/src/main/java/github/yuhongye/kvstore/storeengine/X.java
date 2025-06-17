package github.yuhongye.kvstore.storeengine;

/**
 * Binary Structure
 * <table summary="record format">
 * <thead>
 * <tr>
 * <th>name</th>
 * <th>offset</th>
 * <th>length</th>
 * <th>description</th>
 * </tr>
 * </thead>
 * <tbody>
 * <tr>
 * <td>shared key length</td>
 * <td>0</td>
 * <td>vary</td>
 * <td>variable length encoded int: size of shared key prefix with the key from the previous entry</td>
 * </tr>
 * <tr>
 * <td>non-shared key length</td>
 * <td>vary</td>
 * <td>vary</td>
 * <td>variable length encoded int: size of non-shared key suffix in this entry</td>
 * </tr>
 * <tr>
 * <td>value length</td>
 * <td>vary</td>
 * <td>vary</td>
 * <td>variable length encoded int: size of value in this entry</td>
 * </tr>
 * <tr>
 * <td>non-shared key</td>
 * <td>vary</td>
 * <td>non-shared key length</td>
 * <td>non-shared key data</td>
 * </tr>
 * <tr>
 * <td>value</td>
 * <td>vary</td>
 * <td>value length</td>
 * <td>value data</td>
 * </tr>
 * </tbody>
 * </table>
 */
public class X {
}
