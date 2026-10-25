import javax.crypto.Cipher;
import javax.crypto.SecretKey;
import javax.crypto.SecretKeyFactory;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.PBEKeySpec;
import javax.crypto.spec.SecretKeySpec;
import java.nio.charset.StandardCharsets;
import java.security.GeneralSecurityException;
import java.security.SecureRandom;
import java.util.Base64;

public class Main {
    public static void main(String[] args) throws GeneralSecurityException {
        String plaintext = args.length > 0 ? args[0] : "Hello, World!";
        String password = args.length > 1 ? args[1] : "change-me";

        String encrypted = encrypt(plaintext, password);

        System.out.println(encrypted);
    }

    public static String encrypt(String plaintext, String password) throws GeneralSecurityException {
        SecureRandom random = new SecureRandom();

        byte[] salt = new byte[16];
        random.nextBytes(salt);

        byte[] iv = new byte[16];
        random.nextBytes(iv);

        SecretKey key = deriveKey(password, salt);

        Cipher cipher = Cipher.getInstance("AES/CBC/PKCS5Padding");
        cipher.init(Cipher.ENCRYPT_MODE, key, new IvParameterSpec(iv));

        byte[] ciphertext = cipher.doFinal(plaintext.getBytes(StandardCharsets.UTF_8));

        byte[] output = new byte[salt.length + iv.length + ciphertext.length];
        System.arraycopy(salt, 0, output, 0, salt.length);
        System.arraycopy(iv, 0, output, salt.length, iv.length);
        System.arraycopy(ciphertext, 0, output, salt.length + iv.length, ciphertext.length);

        return Base64.getEncoder().encodeToString(output);
    }

    private static SecretKey deriveKey(String password, byte[] salt) throws GeneralSecurityException {
        PBEKeySpec spec = new PBEKeySpec(
                password.toCharArray(),
                salt,
                100000,
                256
        );

        SecretKeyFactory factory = SecretKeyFactory.getInstance("PBKDF2WithHmacSHA256");
        byte[] keyBytes = factory.generateSecret(spec).getEncoded();

        return new SecretKeySpec(keyBytes, "AES");
    }
}