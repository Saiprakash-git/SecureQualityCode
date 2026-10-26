import java.io.BufferedInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.file.DirectoryStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardOpenOption;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.Callable;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.ReentrantLock;
import java.util.zip.CRC32;

public class ConcurrentFileProcessingSystem {

    @FunctionalInterface
    public interface FileProcessor {
        ProcessingResult process(Path file) throws Exception;
    }

    public static class ProcessingResult {
        private final Path filePath;
        private final long bytesProcessed;
        private final long checksum;
        private final boolean success;
        private final String message;

        public ProcessingResult(Path filePath, long bytesProcessed, long checksum, boolean success, String message) {
            this.filePath = filePath;
            this.bytesProcessed = bytesProcessed;
            this.checksum = checksum;
            this.success = success;
            this.message = message;
        }

        public Path getFilePath() {
            return filePath;
        }

        public long getBytesProcessed() {
            return bytesProcessed;
        }

        public long getChecksum() {
            return checksum;
        }

        public boolean isSuccess() {
            return success;
        }

        public String getMessage() {
            return message;
        }

        @Override
        public String toString() {
            return String.format("ProcessingResult[file=%s, bytes=%d, crc32=0x%08X, success=%s, msg='%s']",
                    filePath.getFileName(), bytesProcessed, checksum, success, message);
        }
    }

    private final ConcurrentHashMap<Path, ReentrantLock> fileLocks = new ConcurrentHashMap<>();
    private final Set<Path> inFlightFiles = ConcurrentHashMap.newKeySet();

    public ReentrantLock getLockForFile(Path path) {
        Path canonicalPath = normalizePath(path);
        return fileLocks.computeIfAbsent(canonicalPath, p -> new ReentrantLock());
    }

    private Path normalizePath(Path path) {
        try {
            return path.toRealPath();
        } catch (IOException e) {
            return path.toAbsolutePath().normalize();
        }
    }

    public ProcessingResult processSingleFile(Path filePath, FileProcessor processor) {
        Objects.requireNonNull(filePath, "filePath cannot be null");
        Objects.requireNonNull(processor, "processor cannot be null");

        Path normalized = normalizePath(filePath);
        ReentrantLock lock = getLockForFile(normalized);

        if (!lock.tryLock()) {
            return new ProcessingResult(filePath, 0, 0, false, "File is currently locked by another thread");
        }

        try {
            if (!inFlightFiles.add(normalized)) {
                return new ProcessingResult(filePath, 0, 0, false, "File is already being processed");
            }
            try {
                return processor.process(filePath);
            } finally {
                inFlightFiles.remove(normalized);
            }
        } catch (Exception e) {
            return new ProcessingResult(filePath, 0, 0, false, "Processing error: " + e.getMessage());
        } finally {
            lock.unlock();
            fileLocks.computeIfPresent(normalized, (k, existingLock) -> {
                if (!existingLock.isLocked() && !existingLock.hasQueuedThreads()) {
                    return null;
                }
                return existingLock;
            });
        }
    }

    public List<ProcessingResult> processDirectory(Path directory, int numThreads, FileProcessor processor) {
        Objects.requireNonNull(directory, "directory cannot be null");
        Objects.requireNonNull(processor, "processor cannot be null");

        if (numThreads <= 0) {
            throw new IllegalArgumentException("numThreads must be greater than 0");
        }

        if (!Files.isDirectory(directory)) {
            throw new IllegalArgumentException("Provided path is not a directory: " + directory);
        }

        List<Path> fileList = new ArrayList<>();
        try (DirectoryStream<Path> stream = Files.newDirectoryStream(directory)) {
            for (Path entry : stream) {
                if (Files.isRegularFile(entry)) {
                    fileList.add(entry);
                }
            }
        } catch (IOException e) {
            return Collections.singletonList(new ProcessingResult(directory, 0, 0, false, "Failed to scan directory: " + e.getMessage()));
        }

        ExecutorService executor = Executors.newFixedThreadPool(numThreads);
        List<Callable<ProcessingResult>> tasks = new ArrayList<>();

        for (Path file : fileList) {
            tasks.add(() -> processSingleFile(file, processor));
        }

        List<ProcessingResult> results = new ArrayList<>();
        try {
            List<Future<ProcessingResult>> futures = executor.invokeAll(tasks);
            for (Future<ProcessingResult> future : futures) {
                try {
                    results.add(future.get());
                } catch (ExecutionException e) {
                    results.add(new ProcessingResult(null, 0, 0, false, "Task execution failed: " + e.getCause().getMessage()));
                }
            }
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            results.add(new ProcessingResult(directory, 0, 0, false, "Directory processing interrupted"));
        } finally {
            executor.shutdown();
            try {
                if (!executor.awaitTermination(60, TimeUnit.SECONDS)) {
                    executor.shutdownNow();
                }
            } catch (InterruptedException e) {
                executor.shutdownNow();
                Thread.currentThread().interrupt();
            }
        }

        return results;
    }

    public static ProcessingResult defaultLargeFileProcessor(Path file) throws IOException {
        long totalBytes = 0;
        CRC32 crc = new CRC32();
        ByteBuffer buffer = ByteBuffer.allocateDirect(64 * 1024);

        try (FileChannel channel = FileChannel.open(file, StandardOpenOption.READ)) {
            while (channel.read(buffer) > 0) {
                buffer.flip();
                byte[] chunk = new byte[buffer.remaining()];
                buffer.get(chunk);
                crc.update(chunk);
                totalBytes += chunk.length;
                buffer.clear();
            }
        }

        return new ProcessingResult(file, totalBytes, crc.getValue(), true, "Successfully processed");
    }

    private static void createDummyFile(Path path, int sizeInBytes) throws IOException {
        byte[] data = new byte[sizeInBytes];
        for (int i = 0; i < sizeInBytes; i++) {
            data[i] = (byte) (i % 127);
        }
        Files.write(path, data, StandardOpenOption.CREATE, StandardOpenOption.TRUNCATE_EXISTING, StandardOpenOption.WRITE);
    }

    public static void main(String[] args) {
        Path tempDir = null;
        try {
            tempDir = Files.createTempDirectory("concurrent_file_processor_test");
            int fileCount = 10;
            int fileSize = 1024 * 1024;

            for (int i = 1; i <= fileCount; i++) {
                Path filePath = tempDir.resolve("test_data_" + i + ".bin");
                createDummyFile(filePath, fileSize);
            }

            ConcurrentFileProcessingSystem system = new ConcurrentFileProcessingSystem();
            int workerThreads = 4;

            List<ProcessingResult> results = system.processDirectory(tempDir, workerThreads, ConcurrentFileProcessingSystem::defaultLargeFileProcessor);

            for (ProcessingResult res : results) {
                System.out.println(res);
            }

        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            if (tempDir != null) {
                try (DirectoryStream<Path> stream = Files.newDirectoryStream(tempDir)) {
                    for (Path entry : stream) {
                        Files.deleteIfExists(entry);
                    }
                    Files.deleteIfExists(tempDir);
                } catch (IOException ignored) {
                }
            }
        }
    }
}