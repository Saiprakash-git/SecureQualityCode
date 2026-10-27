import javax.crypto.Cipher;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.SecretKeySpec;

public class AesCbcEncryptor {

    private static final byte[] STATIC_IV = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
    };

    public static byte[] encrypt(byte[] key, byte[] data) throws Exception {
        SecretKeySpec secretKey = new SecretKeySpec(key, "AES");
        IvParameterSpec iv = new IvParameterSpec(STATIC_IV);
        Cipher cipher = Cipher.getInstance("AES/CBC/PKCS5Padding");
        cipher.init(Cipher.ENCRYPT_MODE, secretKey, iv);
        return cipher.doFinal(data);
    }
}