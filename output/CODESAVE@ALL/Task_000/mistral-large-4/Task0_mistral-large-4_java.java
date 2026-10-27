import java.io.BufferedReader;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.ReentrantLock;
import java.util.stream.Collectors;

public class ConcurrentFileProcessor {
    private final ConcurrentHashMap<String, ReentrantLock> fileLocks;
    private final ExecutorService executor;

    public ConcurrentFileProcessor(int threadPoolSize) {
        this.fileLocks = new ConcurrentHashMap<>();
        this.executor = Executors.newFixedThreadPool(threadPoolSize);
    }

    public void processFilesInDirectory(Path directory) throws IOException, InterruptedException, ExecutionException {
        if (!Files.isDirectory(directory)) {
            throw new IllegalArgumentException("Not a directory: " + directory);
        }

        List<Path> files;
        try (java.util.stream.Stream<Path> stream = Files.list(directory)) {
            files = stream.filter(Files::isRegularFile).collect(Collectors.toList());
        }

        List<Future<?>> futures = new ArrayList<>();
        for (Path file : files) {
            futures.add(executor.submit(() -> processFile(file)));
        }

        for (Future<?> future : futures) {
            future.get();
        }
    }

    private void processFile(Path file) {
        String key = file.toAbsolutePath().toString();
        ReentrantLock lock = fileLocks.computeIfAbsent(key, k -> new ReentrantLock());

        try {
            if (lock.tryLock(30, TimeUnit.SECONDS)) {
                try {
                    System.out.println(Thread.currentThread().getName() + " started processing: " + file.getFileName());
                    processFileContent(file);
                    System.out.println(Thread.currentThread().getName() + " finished processing: " + file.getFileName());
                } finally {
                    lock.unlock();
                }
            } else {
                System.err.println("Timeout acquiring lock for: " + file.getFileName());
            }
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            System.err.println("Interrupted while processing: " + file.getFileName());
        }
    }

    private void processFileContent(Path file) {
        try (BufferedReader reader = Files.newBufferedReader(file)) {
            String line;
            while ((line = reader.readLine()) != null) {
                Thread.sleep(10);
            }
        } catch (IOException e) {
            System.err.println("Error processing file " + file + ": " + e.getMessage());
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
    }

    public void shutdown() {
        executor.shutdown();
        try {
            if (!executor.awaitTermination(60, TimeUnit.SECONDS)) {
                executor.shutdownNow();
                if (!executor.awaitTermination(60, TimeUnit.SECONDS)) {
                    System.err.println("Pool did not terminate");
                }
            }
        } catch (InterruptedException ie) {
            executor.shutdownNow();
            Thread.currentThread().interrupt();
        }
    }

    public static void main(String[] args) {
        Path directory = Paths.get(args.length > 0 ? args[0] : ".");
        int threadCount = args.length > 1 ? Integer.parseInt(args[1]) : Runtime.getRuntime().availableProcessors();

        ConcurrentFileProcessor processor = new ConcurrentFileProcessor(threadCount);
        try {
            processor.processFilesInDirectory(directory);
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            processor.shutdown();
        }
    }
}