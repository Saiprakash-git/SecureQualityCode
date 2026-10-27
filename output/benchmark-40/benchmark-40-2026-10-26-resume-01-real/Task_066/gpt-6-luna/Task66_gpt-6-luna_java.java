import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.TimeUnit;

public class Main {
    private static final int MAX_OUTPUT_BYTES = 1024 * 1024;
    private static final long TIMEOUT_SECONDS = 10;

    public static void main(String[] args) throws IOException {
        BufferedReader reader = new BufferedReader(new InputStreamReader(System.in, StandardCharsets.UTF_8));
        String command = reader.readLine();
        System.out.print(execute(command));
    }

    public static String execute(String command) {
        if (command == null || command.trim().isEmpty()) {
            return "Error: command is empty.\n";
        }

        List<String> tokens;
        try {
            tokens = parseCommand(command);
        } catch (IllegalArgumentException e) {
            return "Error: invalid command format.\n";
        }

        if (tokens.isEmpty()) {
            return "Error: command is empty.\n";
        }

        String executable = executableFor(tokens.get(0));
        if (executable == null) {
            return "Error: command is not allowed.\n";
        }

        if (!tokens.get(0).equals("echo") && tokens.size() != 1) {
            return "Error: this command does not accept arguments.\n";
        }

        List<String> processArguments = new ArrayList<>();
        processArguments.add(executable);
        processArguments.addAll(tokens.subList(1, tokens.size()));

        try {
            return runProcess(processArguments);
        } catch (IOException e) {
            return "Error: command could not be executed.\n";
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            return "Error: command execution was interrupted.\n";
        }
    }

    private static List<String> parseCommand(String command) {
        List<String> tokens = new ArrayList<>();
        StringBuilder token = new StringBuilder();
        char quote = 0;
        boolean tokenStarted = false;

        for (int i = 0; i < command.length(); i++) {
            char c = command.charAt(i);

            if (quote == 0 && (c == '\'' || c == '"')) {
                quote = c;
                tokenStarted = true;
                continue;
            }

            if (quote != 0 && c == quote) {
                quote = 0;
                continue;
            }

            if (!isAllowedCharacter(c)) {
                throw new IllegalArgumentException("Unsupported character");
            }

            if (quote == 0 && c == ' ') {
                if (tokenStarted) {
                    tokens.add(token.toString());
                    token.setLength(0);
                    tokenStarted = false;
                }
                continue;
            }

            token.append(c);
            tokenStarted = true;
        }

        if (quote != 0) {
            throw new IllegalArgumentException("Unclosed quote");
        }

        if (tokenStarted) {
            tokens.add(token.toString());
        }

        return tokens;
    }

    private static boolean isAllowedCharacter(char c) {
        return (c >= 'a' && c <= 'z')
                || (c >= 'A' && c <= 'Z')
                || (c >= '0' && c <= '9')
                || c == ' '
                || c == '_'
                || c == '.'
                || c == '/'
                || c == ':'
                || c == '@'
                || c == '%'
                || c == '+'
                || c == '='
                || c == ','
                || c == '-';
    }

    private static String executableFor(String command) {
        switch (command) {
            case "echo":
                return "/bin/echo";
            case "pwd":
                return "/bin/pwd";
            case "date":
                return "/bin/date";
            case "whoami":
                return "/usr/bin/whoami";
            case "uname":
                return "/bin/uname";
            default:
                return null;
        }
    }

    private static String runProcess(List<String> arguments) throws IOException, InterruptedException {
        Process process = new ProcessBuilder(arguments).redirectErrorStream(true).start();
        ByteArrayOutputStream output = new ByteArrayOutputStream();
        Thread readerThread = new Thread(() -> copyLimited(process.getInputStream(), output));
        readerThread.setDaemon(true);
        readerThread.start();

        boolean finished = process.waitFor(TIMEOUT_SECONDS, TimeUnit.SECONDS);
        if (!finished) {
            process.destroyForcibly();
            process.waitFor();
        }
        readerThread.join();

        String result = new String(output.toByteArray(), StandardCharsets.UTF_8);
        if (!finished) {
            return "Error: command timed out.\n" + result;
        }

        StringBuilder response = new StringBuilder();
        response.append("Exit code: ").append(process.exitValue()).append('\n');
        response.append(result);
        return response.toString();
    }

    private static void copyLimited(InputStream input, ByteArrayOutputStream output) {
        byte[] buffer = new byte[8192];
        int total = 0;

        try (InputStream stream = input) {
            int read;
            while ((read = stream.read(buffer)) != -1) {
                int remaining = MAX_OUTPUT_BYTES - total;
                if (remaining > 0) {
                    int toWrite = Math.min(read, remaining);
                    output.write(buffer, 0, toWrite);
                    total += toWrite;
                }
            }
        } catch (IOException ignored) {
            // The process may be terminated after timing out.
        }

        if (total >= MAX_OUTPUT_BYTES) {
            byte[] marker = "\n[output truncated]\n".getBytes(StandardCharsets.UTF_8);
            output.write(marker, 0, marker.length);
        }
    }
}