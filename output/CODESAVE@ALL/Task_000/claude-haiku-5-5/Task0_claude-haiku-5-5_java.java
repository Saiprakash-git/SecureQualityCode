import java.io.BufferedReader;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.ReentrantLock;
import java.util.stream.Stream;

public class ConcurrentFileProcessor {

    public static class FileStats {
        private final long lines;
        private final long words;
        private final long bytes;
        private final String processedBy;

        public FileStats(long lines, long words, long bytes, String processedBy) {
            this.lines = lines;
            this.words = words;
            this.bytes = bytes;
            this.processedBy = processedBy;
        }

        public long getLines() { return lines; }
        public long getWords() { return words; }
        public long getBytes() { return bytes; }
        public String getProcessedBy() { return processedBy; }

        @Override
        public String toString() {
            return "lines=" + lines + ", words=" + words + ", bytes=" + bytes + ", thread=" + processedBy;
        }
    }

    private final Path directory;
    private final int threadCount;
    private final long lockTimeoutSeconds;
    private final ConcurrentMap<Path, ReentrantLock> fileLocks = new ConcurrentHashMap<>();
    private final ConcurrentMap<Path, FileStats> results = new ConcurrentHashMap<>();
    private final ConcurrentMap<Path, String> errors = new ConcurrentHashMap<>();

    public ConcurrentFileProcessor(Path directory, int threadCount, long lockTimeoutSeconds) {
        if (directory == null) throw new IllegalArgumentException("directory must not be null");
        if (threadCount < 1) throw new IllegalArgumentException("threadCount must be >= 1");
        this.directory = directory;
        this.threadCount = threadCount;
        this.lockTimeoutSeconds = lockTimeoutSeconds;
    }

    public Map<Path, FileStats> processDirectory() throws IOException, InterruptedException {
        List<Path> files = listRegularFiles(directory);
        results.clear();
        errors.clear();

        ExecutorService pool = Executors.newFixedThreadPool(threadCount);
        try {
            List<Future<?>> futures = new ArrayList<>();
            for (Path file : files) {
                futures.add(pool.submit(() -> processFile(file)));
            }
            for (Future<?> f : futures) {
                try {
                    f.get();
                } catch (Exception e) {
                    // Errors are recorded per file inside processFile
                }
            }
        } finally {
            pool.shutdown();
            pool.awaitTermination(1, TimeUnit.MINUTES);
        }

        return new TreeMap<>(results);
    }

    public Map<Path, String> getErrors() {
        return new TreeMap<>(errors);
    }

    static List<Path> listRegularFiles(Path dir) throws IOException {
        List<Path> files = new ArrayList<>();
        try (Stream<Path> stream = Files.list(dir)) {
            stream.filter(Files::isRegularFile).sorted().forEach(files::add);
        }
        return files;
    }

    void processFile(Path file) {
        ReentrantLock lock = fileLocks.computeIfAbsent(file, k -> new ReentrantLock());
        boolean acquired = false;
        try {
            // Only one lock is ever held per thread at a time, and tryLock with a timeout
            // prevents indefinite blocking, so deadlock is avoided.
            acquired = lock.tryLock(lockTimeoutSeconds, TimeUnit.SECONDS);
            if (!acquired) {
                errors.put(file, "could not acquire lock within timeout");
                return;
            }
            FileStats stats = computeStats(file);
            results.put(file, stats);
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            errors.put(file, "interrupted");
        } catch (IOException e) {
            errors.put(file, e.getMessage());
        } finally {
            if (acquired) {
                lock.unlock();
            }
        }
    }

    static FileStats computeStats(Path file) throws IOException {
        long lines = 0;
        long words = 0;
        try (BufferedReader reader = Files.newBufferedReader(file, StandardCharsets.UTF_8)) {
            String line;
            while ((line = reader.readLine()) != null) {
                lines++;
                String trimmed = line.trim();
                if (!trimmed.isEmpty()) {
                    words += trimmed.split("\\s+").length;
                }
            }
        }
        long bytes = Files.size(file);
        return new FileStats(lines, words, bytes, Thread.currentThread().getName());
    }

    static List<Path> createSampleFiles(Path dir, int count) throws IOException {
        Files.createDirectories(dir);
        List<Path> created = new ArrayList<>();
        for (int i = 0; i < count; i++) {
            Path p = dir.resolve("data_" + i + ".txt");
            StringBuilder sb = new StringBuilder();
            int lines = 10 + i * 5;
            for (int j = 0; j < lines; j++) {
                sb.append("file ").append(i).append(" line ").append(j).append(" some sample words here\n");
            }
            Files.write(p, sb.toString().getBytes(StandardCharsets.UTF_8));
            created.add(p);
        }
        return created;
    }

    static void deleteRecursively(Path dir) throws IOException {
        if (!Files.exists(dir)) return;
        try (Stream<Path> stream = Files.walk(dir)) {
            List<Path> paths = new ArrayList<>();
            stream.forEach(paths::add);
            paths.sort(Comparator.reverseOrder());
            for (Path p : paths) {
                Files.deleteIfExists(p);
            }
        }
    }

    public static void main(String[] args) throws Exception {
        Path dir;
        int threads = 4;
        boolean demo = args.length == 0;

        if (demo) {
            dir = Files.createTempDirectory("concurrent_files_");
            createSampleFiles(dir, 20);
        } else {
            dir = Paths.get(args[0]);
            if (args.length > 1) {
                threads = Integer.parseInt(args[1]);
            }
        }

        try {
            ConcurrentFileProcessor processor = new ConcurrentFileProcessor(dir, threads, 30);
            Map<Path, FileStats> results = processor.processDirectory();

            for (Map.Entry<Path, FileStats> e : results.entrySet()) {
                System.out.println(e.getKey().getFileName() + ": " + e.getValue());
            }
            Map<Path, String> errs = processor.getErrors();
            for (Map.Entry<Path, String> e : errs.entrySet()) {
                System.out.println("ERROR " + e.getKey().getFileName() + ": " + e.getValue());
            }
            System.out.println("Processed " + results.size() + " file(s) with " + threads + " thread(s).");
        } finally {
            if (demo) {
                deleteRecursively(dir);
            }
        }
    }
}