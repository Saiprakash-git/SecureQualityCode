import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.attribute.PosixFilePermission;
import java.util.HashSet;
import java.util.Set;

public class CreateScript {
    public static boolean createExecutableScript(String filename) {
        try {
            Path path = Paths.get(filename);
            if (!Files.exists(path)) {
                Files.createFile(path);
            }
            Set<PosixFilePermission> perms = new HashSet<>();
            perms.add(PosixFilePermission.OWNER_READ);
            perms.add(PosixFilePermission.OWNER_WRITE);
            perms.add(PosixFilePermission.OWNER_EXECUTE);
            perms.add(PosixFilePermission.GROUP_READ);
            perms.add(PosixFilePermission.GROUP_EXECUTE);
            perms.add(PosixFilePermission.OTHERS_READ);
            perms.add(PosixFilePermission.OTHERS_EXECUTE);
            Files.setPosixFilePermissions(path, perms);
            return true;
        } catch (IOException e) {
            return false;
        }
    }

    public static void main(String[] args) {
        createExecutableScript("script.sh");
    }
}