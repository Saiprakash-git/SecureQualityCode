import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Scanner;
import java.util.Set;
import java.util.concurrent.TimeUnit;
import java.util.regex.Pattern;

public class BashCommandExecutor {

    private static final Set<String> ALLOWED_COMMANDS = new HashSet<>(Arrays.asList(
            "echo", "ls", "pwd", "date", "whoami", "uname", "uptime",
            "cat", "grep", "wc", "head", "tail", "sort", "uniq", "tr"
    ));

    private static final Pattern SAFE_CHARACTER_PATTERN = Pattern.compile("^[a-zA-Z0-9_./\\-\\s\"'|=><:]+$");
    private static final Pattern FORBIDDEN_PATTERNS = Pattern.compile("(`|\\$\\([^)]*\\)|;|&&|\\|\\||\\\\)");

    public static class ExecutionResult {
        private final int exitCode;
        private final String stdout;
        private final String stderr;

        public ExecutionResult(int exitCode, String stdout, String stderr) {
            this.exitCode = exitCode;
            this.stdout = stdout;
            this.stderr = stderr;
        }

        public int getExitCode() {
            return exitCode;
        }

        public String getStdout() {
            return stdout;
        }

        public String getStderr() {
            return stderr;
        }

        @Override
        public String toString() {
            return String.format("Exit Code: %d%n--- STDOUT ---%n%s%n--- STDERR ---%n%s",
                    exitCode, stdout.isEmpty() ? "(empty)" : stdout, stderr.isEmpty() ? "(empty)" : stderr);
        }
    }

    public static boolean validateCommand(String command) {
        if (command == null || command.trim().isEmpty()) {
            return false;
        }

        String trimmed = command.trim();

        if (trimmed.length() > 500) {
            return false;
        }

        if (!SAFE_CHARACTER_PATTERN.matcher(trimmed).matches()) {
            return false;
        }

        if (FORBIDDEN_PATTERNS.matcher(trimmed).find()) {
            return false;
        }

        String[] pipelineSegments = trimmed.split("\\|");
        for (String segment : pipelineSegments) {
            String segmentTrimmed = segment.trim();
            if (segmentTrimmed.isEmpty()) {
                return false;
            }

            String[] tokens = segmentTrimmed.split("\\s+");
            String baseCommand = tokens[0];

            if (baseCommand.contains("/")) {
                String[] pathParts = baseCommand.split("/");
                baseCommand = pathParts[pathParts.length - 1];
            }

            if (!ALLOWED_COMMANDS.contains(baseCommand)) {
                return false;
            }
        }

        return true;
    }

    public static ExecutionResult executeCommand(String command, long timeoutSeconds) throws Exception {
        if (!validateCommand(command)) {
            throw new IllegalArgumentException("Command validation failed: command is empty, malformed, or contains unauthorized instructions.");
        }

        ProcessBuilder processBuilder = new ProcessBuilder("bash", "-c", command);
        processBuilder.environment().clear();
        processBuilder.environment().put("PATH", "/usr/bin:/bin");

        Process process = processBuilder.start();

        StringBuilder stdoutOutput = new StringBuilder();
        StringBuilder stderrOutput = new StringBuilder();

        Thread stdoutReader = new Thread(() -> {
            try (BufferedReader reader = new BufferedReader(new InputStreamReader(process.getInputStream(), StandardCharsets.UTF_8))) {
                String line;
                while ((line = reader.readLine()) != null) {
                    stdoutOutput.append(line).append(System.lineSeparator());
                }
            } catch (Exception ignored) {
            }
        });

        Thread stderrReader = new Thread(() -> {
            try (BufferedReader reader = new BufferedReader(new InputStreamReader(process.getErrorStream(), StandardCharsets.UTF_8))) {
                String line;
                while ((line = reader.readLine()) != null) {
                    stderrOutput.append(line).append(System.lineSeparator());
                }
            } catch (Exception ignored) {
            }
        });

        stdoutReader.start();
        stderrReader.start();

        boolean finished = process.waitFor(timeoutSeconds, TimeUnit.SECONDS);

        if (!finished) {
            process.destroyForcibly();
            stdoutReader.interrupt();
            stderrReader.interrupt();
            throw new RuntimeException("Command execution timed out after " + timeoutSeconds + " seconds.");
        }

        stdoutReader.join();
        stderrReader.join();

        int exitCode = process.exitValue();
        return new ExecutionResult(exitCode, stdoutOutput.toString().trim(), stderrOutput.toString().trim());
    }

    public static void main(String[] args) {
        Scanner scanner = new Scanner(System.in);
        System.out.println("Enter a bash command to execute (allowed: echo, ls, pwd, date, whoami, uname, uptime, cat, grep, wc, head, tail, sort, uniq, tr):");

        if (scanner.hasNextLine()) {
            String inputCommand = scanner.nextLine();
            try {
                ExecutionResult result = executeCommand(inputCommand, 10);
                System.out.println(result);
            } catch (Exception e) {
                System.err.println("Error: " + e.getMessage());
            }
        }
        scanner.close();
    }
}