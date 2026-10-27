import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

public class Main {
    public static void main(String[] args) throws IOException {
        Path script = Paths.get("script.sh");
        Files.write(script, "#!/bin/sh\n".getBytes(StandardCharsets.UTF_8));

        if (!script.toFile().setExecutable(true, false)) {
            throw new IOException("Could not make script.sh executable");
        }
    }
}