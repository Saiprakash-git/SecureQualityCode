import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.attribute.PosixFilePermission;
import java.util.HashSet;
import java.util.Set;

public class CreateExecutableScript {

    public static void main(String[] args) {
        String fileName = args.length > 0 ? args[0] : "script.sh";
        String content = "#!/bin/sh\necho \"Executing script.sh\"\n";
        boolean success = createAndMakeExecutable(fileName, content);
        System.exit(success ? 0 : 1);
    }

    public static boolean createAndMakeExecutable(String fileName, String content) {
        try {
            Path path = Paths.get(fileName);
            Files.writeString(path, content);

            try {
                Set<PosixFilePermission> permissions = new HashSet<>(Files.getPosixFilePermissions(path));
                permissions.add(PosixFilePermission.OWNER_EXECUTE);
                permissions.add(PosixFilePermission.GROUP_EXECUTE);
                permissions.add(PosixFilePermission.OTHERS_EXECUTE);
                Files.setPosixFilePermissions(path, permissions);
            } catch (UnsupportedOperationException e) {
                File file = path.toFile();
                file.setExecutable(true, false);
            }

            return true;
        } catch (IOException e) {
            return false;
        }
    }
}