import java.io.IOException;
import java.io.InputStream;
import java.nio.file.DirectoryStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.locks.ReentrantLock;

public class ConcurrentFileProcessor {
    private static final ConcurrentMap<Path, ReentrantLock> FILE_LOCKS =
            new ConcurrentHashMap<Path, ReentrantLock>();

    @FunctionalInterface
    public interface FileTask<T> {
        T process(Path file) throws Exception;
    }

    public static final class FileResult<T> {
        private final Path file;
        private final T value;
        private final Exception error;

        private FileResult(Path file, T value, Exception error) {
            this.file = file;
            this.value = value;
            this.error = error;
        }

        public Path getFile() {
            return file;
        }

        public T getValue() {
            return value;
        }

        public Exception getError() {
            return error;
        }

        public boolean isSuccessful() {
            return error == null;
        }
    }

    public static <T> List<FileResult<T>> processDirectory(
            Path directory, int threadCount, FileTask<T> task)
            throws IOException, InterruptedException {
        if (directory == null || task == null) {
            throw new IllegalArgumentException("Directory and task must not be null");
        }
        if (threadCount < 1) {
            throw new IllegalArgumentException("Thread count must be at least 1");
        }

        List<Path> files = new ArrayList<Path>();
        try (DirectoryStream<Path> entries = Files.newDirectoryStream(directory)) {
            for (Path entry : entries) {
                if (Files.isRegularFile(entry)) {
                    files.add(entry);
                }
            }
        }
        Collections.sort(files, Comparator.comparing(Path::toString));

        ExecutorService executor = Executors.newFixedThreadPool(threadCount);
        boolean completed = false;
        try {
            List<Future<FileResult<T>>> futures = new ArrayList<Future<FileResult<T>>>();
            for (final Path file : files) {
                futures.add(executor.submit(new Callable<FileResult<T>>() {
                    @Override
                    public FileResult<T> call() {
                        Path key;
                        try {
                            key = file.toRealPath();
                        } catch (IOException e) {
                            key = file.toAbsolutePath().normalize();
                        }

                        ReentrantLock lock = FILE_LOCKS.computeIfAbsent(
                                key, ignored -> new ReentrantLock());
                        lock.lock();
                        try {
                            return new FileResult<T>(file, task.process(file), null);
                        } catch (Exception e) {
                            return new FileResult<T>(file, null, e);
                        } finally {
                            lock.unlock();
                        }
                    }
                }));
            }

            executor.shutdown();
            List<FileResult<T>> results = new ArrayList<FileResult<T>>(futures.size());
            for (Future<FileResult<T>> future : futures) {
                try {
                    results.add(future.get());
                } catch (ExecutionException e) {
                    Throwable cause = e.getCause();
                    Exception error = cause instanceof Exception
                            ? (Exception) cause
                            : new RuntimeException(cause);
                    results.add(new FileResult<T>(null, null, error));
                }
            }
            completed = true;
            return results;
        } finally {
            if (!completed) {
                executor.shutdownNow();
            }
        }
    }

    private static String sha256(Path file) throws Exception {
        MessageDigest digest = MessageDigest.getInstance("SHA-256");
        byte[] buffer = new byte[64 * 1024];
        try (InputStream input = Files.newInputStream(file)) {
            int count;
            while ((count = input.read(buffer)) != -1) {
                digest.update(buffer, 0, count);
            }
        }

        StringBuilder result = new StringBuilder();
        for (byte value : digest.digest()) {
            result.append(String.format("%02x", value & 0xff));
        }
        return result.toString();
    }

    public static void main(String[] args) throws Exception {
        if (args.length < 1 || args.length > 2) {
            System.err.println("Usage: java ConcurrentFileProcessor <directory> [thread-count]");
            return;
        }

        Path directory = Paths.get(args[0]);
        int threadCount = args.length == 2
                ? Integer.parseInt(args[1])
                : Runtime.getRuntime().availableProcessors();

        List<FileResult<String>> results = processDirectory(
                directory, threadCount, ConcurrentFileProcessor::sha256);

        for (FileResult<String> result : results) {
            if (result.isSuccessful()) {
                System.out.println(result.getFile() + " " + result.getValue());
            } else {
                System.err.println(result.getFile() + " failed: "
                        + result.getError().getMessage());
            }
        }
    }
}