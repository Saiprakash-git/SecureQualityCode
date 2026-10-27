import javax.crypto.Cipher;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.SecretKeySpec;
import java.security.GeneralSecurityException;
import java.util.Arrays;

public class Main {
    private static final int AES_BLOCK_SIZE = 16;

    public static byte[] createStaticIV() {
        return new byte[AES_BLOCK_SIZE];
    }

    public static byte[] pad(byte[] data, int blockSize) {
        if (data == null) {
            throw new IllegalArgumentException("data must not be null");
        }
        if (blockSize <= 0) {
            throw new IllegalArgumentException("blockSize must be positive");
        }

        int padLength = blockSize - (data.length % blockSize);
        byte[] padded = Arrays.copyOf(data, data.length + padLength);

        for (int i = data.length; i < padded.length; i++) {
            padded[i] = (byte) padLength;
        }

        return padded;
    }

    public static byte[] encrypt(byte[] data, byte[] key) throws GeneralSecurityException {
        if (data == null) {
            throw new IllegalArgumentException("data must not be null");
        }
        if (key == null) {
            throw new IllegalArgumentException("key must not be null");
        }

        byte[] iv = createStaticIV();
        SecretKeySpec keySpec = new SecretKeySpec(key, "AES");
        IvParameterSpec ivSpec = new IvParameterSpec(iv);

        Cipher cipher = Cipher.getInstance("AES/CBC/NoPadding");
        cipher.init(Cipher.ENCRYPT_MODE, keySpec, ivSpec);

        int blockSize = cipher.getBlockSize();
        if (blockSize <= 0) {
            blockSize = AES_BLOCK_SIZE;
        }

        byte[] paddedData = pad(data, blockSize);
        return cipher.doFinal(paddedData);
    }
}