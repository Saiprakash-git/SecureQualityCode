import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;
import java.util.regex.Pattern;

public class ShellCommandRunner {

    private static final int MAX_COMMAND_LENGTH = 1024;
    private static final long TIMEOUT_SECONDS = 10;

    private static final List<Pattern> BLOCKED_PATTERNS = List.of(
            Pattern.compile("(^|[;&|(\\s])sudo\\b"),
            Pattern.compile("(^|[;&|(\\s])su\\b"),
            Pattern.compile("\\bmkfs(\\.\\w+)?\\b"),
            Pattern.compile("\\b(shutdown|reboot|halt|poweroff|init\\s+0)\\b"),
            Pattern.compile("\\bdd\\s+.*\\bof=/dev/"),
            Pattern.compile("\\brm\\s+(-\\S*\\s+)*-\\S*[rR]\\S*\\s+(/|~|\\*|\\$HOME)(\\s|$)"),
            Pattern.compile(">\\s*/dev/(sd|hd|nvme|disk)"),
            Pattern.compile(":\\(\\)\\s*\\{.*\\}\\s*;?\\s*:"),
            Pattern.compile("\\bchmod\\s+(-R\\s+)?[0-7]*777\\s+/(\\s|$)"),
            Pattern.compile("\\bchown\\s+-R\\b.*\\s/(\\s|$)"),
            Pattern.compile("\\bkill\\s+-9\\s+-1\\b"),
            Pattern.compile("/etc/(shadow|sudoers)\\b"),
            Pattern.compile("\\b(curl|wget)\\b[^|]*\\|\\s*(ba|z)?sh\\b"),
            Pattern.compile("\\bmv\\s+/\\s"),
            Pattern.compile("\\bnc\\b.*\\s-[a-zA-Z]*e\\b")
    );

    public static String validate(String command) {
        if (command == null) {
            return "Command is null.";
        }
        String trimmed = command.trim();
        if (trimmed.isEmpty()) {
            return "Command is empty.";
        }
        if (trimmed.length() > MAX_COMMAND_LENGTH) {
            return "Command exceeds maximum length of " + MAX_COMMAND_LENGTH + " characters.";
        }
        if (command.indexOf('\n') >= 0 || command.indexOf('\r') >= 0) {
            return "Command must be a single line.";
        }
        for (int i = 0; i < command.length(); i++) {
            char c = command.charAt(i);
            if (Character.isISOControl(c) && c != '\t') {
                return "Command contains invalid control character at position " + i + ".";
            }
        }
        String quoteError = checkQuotes(command);
        if (quoteError != null) {
            return quoteError;
        }
        for (Pattern p : BLOCKED_PATTERNS) {
            if (p.matcher(command).find()) {
                return "Command matches a disallowed pattern: " + p.pattern();
            }
        }
        return null;
    }

    static String checkQuotes(String command) {
        boolean inSingle = false;
        boolean inDouble = false;
        for (int i = 0; i < command.length(); i++) {
            char c = command.charAt(i);
            if (c == '\\' && !inSingle) {
                i++;
                continue;
            }
            if (c == '\'' && !inDouble) {
                inSingle = !inSingle;
            } else if (c == '"' && !inSingle) {
                inDouble = !inDouble;
            }
        }
        if (inSingle) {
            return "Unclosed single quote.";
        }
        if (inDouble) {
            return "Unclosed double quote.";
        }
        return null;
    }

    public static ExecutionResult execute(String command, long timeoutSeconds)
            throws IOException, InterruptedException {
        ProcessBuilder pb = new ProcessBuilder("/bin/bash", "-c", command);
        pb.directory(new File(System.getProperty("user.dir")));
        Process process = pb.start();
        process.getOutputStream().close();

        CompletableFuture<String> stdoutFuture =
                CompletableFuture.supplyAsync(() -> readStream(process.getInputStream()));
        CompletableFuture<String> stderrFuture =
                CompletableFuture.supplyAsync(() -> readStream(process.getErrorStream()));

        boolean finished = process.waitFor(timeoutSeconds, TimeUnit.SECONDS);
        if (!finished) {
            process.destroyForcibly();
            process.waitFor(5, TimeUnit.SECONDS);
        }

        String stdout = stdoutFuture.join();
        String stderr = stderrFuture.join();
        int exitCode = finished ? process.exitValue() : -1;
        return new ExecutionResult(exitCode, stdout, stderr, !finished);
    }

    private static String readStream(InputStream in) {
        try (ByteArrayOutputStream buffer = new ByteArrayOutputStream()) {
            byte[] chunk = new byte[4096];
            int n;
            while ((n = in.read(chunk)) != -1) {
                buffer.write(chunk, 0, n);
            }
            return buffer.toString(StandardCharsets.UTF_8);
        } catch (IOException e) {
            return "[error reading stream: " + e.getMessage() + "]";
        }
    }

    public static String formatResult(ExecutionResult result) {
        StringBuilder sb = new StringBuilder();
        sb.append("Exit code: ").append(result.exitCode).append('\n');
        if (result.timedOut) {
            sb.append("Status: TIMED OUT after ").append(TIMEOUT_SECONDS).append(" seconds\n");
        }
        sb.append("--- STDOUT ---\n").append(result.stdout);
        if (!result.stdout.endsW