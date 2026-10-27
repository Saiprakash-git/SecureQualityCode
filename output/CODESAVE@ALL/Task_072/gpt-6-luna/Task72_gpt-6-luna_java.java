import javax.crypto.Cipher;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.SecretKeySpec;
import java.security.GeneralSecurityException;

public final class Main {
    private static final byte[] IV = {
        0x00, 0x01, 0x02, 0x03,
        0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B,
        0x0C, 0x0D, 0x0E, 0x0F
    };

    private Main() {
    }

    public static byte[] encrypt(byte[] input, byte[] key) throws GeneralSecurityException {
        if (input == null || key == null) {
            throw new IllegalArgumentException("Input and key must not be null");
        }

        Cipher cipher = Cipher.getInstance("AES/CBC/PKCS5Padding");
        cipher.init(
            Cipher.ENCRYPT_MODE,
            new SecretKeySpec(key, "AES"),
            new IvParameterSpec(IV)
        );
        return cipher.doFinal(input);
    }
}