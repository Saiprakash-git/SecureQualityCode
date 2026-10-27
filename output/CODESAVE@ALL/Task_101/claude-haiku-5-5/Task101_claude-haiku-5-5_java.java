import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Set;
import java.nio.file.attribute.PosixFilePermission;

public class CreateScript {

    public static Path createExecutableScript(String fileName, String content) throws IOException {
        Path path = Paths.get(fileName);
        Files.write(path, content.getBytes(StandardCharsets.UTF_8));

        File file = path.toFile();
        boolean success = file.setExecutable(true, false);
        if (!success) {
            try {
                Set<PosixFilePermission> perms = Files.getPosixFilePermissions(path);
                perms.add(PosixFilePermission.OWNER_EXECUTE);
                perms.add(PosixFilePermission.GROUP_EXECUTE);
                perms.add(PosixFilePermission.OTHERS_EXECUTE);
                Files.setPosixFilePermissions(path, perms);
            } catch (UnsupportedOperationException e) {
                // Non-POSIX file system; setExecutable already attempted
            }
        }
        return path;
    }

    public static void main(String[] args) throws IOException {
        String content = "#!/bin/sh\necho \"Hello from script.sh\"\n";
        Path result = createExecutableScript("script.sh", content);
        System.out.println("Created executable file: " + result.toAbsolutePath());
    }
}