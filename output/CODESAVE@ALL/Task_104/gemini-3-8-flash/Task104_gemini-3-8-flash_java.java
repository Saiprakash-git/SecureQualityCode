import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

public class FixedSizeBufferHandler {

    public static class BoundedBuffer {
        private final char[] buffer;
        private final int capacity;
        private int writeIndex;
        private int readIndex;
        private int count;

        public BoundedBuffer(int capacity) {
            if (capacity <= 0) {
                throw new IllegalArgumentException("Capacity must be greater than zero.");
            }
            this.capacity = capacity;
            this.buffer = new char[capacity];
            this.writeIndex = 0;
            this.readIndex = 0;
            this.count = 0;
        }

        public synchronized int write(char[] source, int offset, int length) {
            if (source == null || offset < 0 || length < 0 || offset + length > source.length) {
                throw new IllegalArgumentException("Invalid source array or bounds.");
            }
            int written = 0;
            while (written < length && count < capacity) {
                buffer[writeIndex] = source[offset + written];
                writeIndex = (writeIndex + 1) % capacity;
                count++;
                written++;
            }
            return written;
        }

        public synchronized int read(char[] destination, int offset, int length) {
            if (destination == null || offset < 0 || length < 0 || offset + length > destination.length) {
                throw new IllegalArgumentException("Invalid destination array or bounds.");
            }
            int readCount = 0;
            while (readCount < length && count > 0) {
                destination[offset + readCount] = buffer[readIndex];
                readIndex = (readIndex + 1) % capacity;
                count--;
                readCount++;
            }
            return readCount;
        }

        public synchronized String drain() {
            char[] drainedChars = new char[count];
            int drainedCount = read(drainedChars, 0, count);
            return new String(drainedChars, 0, drainedCount);
        }

        public synchronized int getCount() {
            return count;
        }

        public synchronized int getCapacity() {
            return capacity;
        }

        public synchronized boolean isFull() {
            return count == capacity;
        }

        public synchronized boolean isEmpty() {
            return count == 0;
        }
    }

    public static List<String> processStreamWithFixedBuffer(InputStream input, int bufferCapacity) throws IOException {
        if (input == null || bufferCapacity <= 0) {
            throw new IllegalArgumentException("Input stream cannot be null and bufferCapacity must be positive.");
        }

        List<String> outputSegments = new ArrayList<>();
        BoundedBuffer boundedBuffer = new BoundedBuffer(bufferCapacity);
        Reader reader = new InputStreamReader(input, StandardCharsets.UTF_8);
        char[] tempChunk = new char[bufferCapacity];
        int charsRead;

        while ((charsRead = reader.read(tempChunk)) != -1) {
            int offset = 0;
            while (offset < charsRead) {
                int written = boundedBuffer.write(tempChunk, offset, charsRead - offset);
                offset += written;

                if (boundedBuffer.isFull()) {
                    outputSegments.add(boundedBuffer.drain());
                }
            }
        }

        if (!boundedBuffer.isEmpty()) {
            outputSegments.add(boundedBuffer.drain());
        }

        return outputSegments;
    }

    public static String appendWithTruncation(BoundedBuffer buffer, String input) {
        if (buffer == null || input == null) {
            throw new IllegalArgumentException("Buffer and input cannot be null.");
        }
        char[] chars = input.toCharArray();
        int written = buffer.write(chars, 0, chars.length);
        if (written < chars.length) {
            return input.substring(0, written);
        }
        return input;
    }

    public static void main(String[] args) {
        try {
            int fixedBufferSize = 8;
            BoundedBuffer manualBuffer = new BoundedBuffer(fixedBufferSize);

            String testInput = "HelloWorldExtendedMessage";
            String accepted = appendWithTruncation(manualBuffer, testInput);
            System.out.println("Buffer capacity: " + manualBuffer.getCapacity());
            System.out.println("Original string: " + testInput);
            System.out.println("Accepted string (fixed-size truncated): " + accepted);
            System.out.println("Buffer content: " + manualBuffer.drain());

            String streamInputData = "This is a stream of input data handled cleanly in fixed chunks.";
            InputStream stream = new ByteArrayInputStream(streamInputData.getBytes(StandardCharsets.UTF_8));
            List<String> segments = processStreamWithFixedBuffer(stream, fixedBufferSize);

            System.out.println("\nProcessed stream in fixed chunks of " + fixedBufferSize + " bytes/chars:");
            for (int i = 0; i < segments.size(); i++) {
                System.out.println("Chunk [" + i + "]: " + segments.get(i));
            }
        } catch (IOException e) {
            System.err.println("Error processing input: " + e.getMessage());
        }
    }
}