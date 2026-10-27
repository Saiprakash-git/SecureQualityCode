import java.io.File;
import java.io.FileWriter;
import java.io.IOException;

public class Main {
    public static void main(String[] args) throws IOException {
        File file = new File("script.sh");

        try (FileWriter writer = new FileWriter(file)) {
            writer.write("#!/bin/sh\n");
            writer.write("echo \"Hello from script.sh\"\n");
        }

        file.setExecutable(true, false);
    }
}