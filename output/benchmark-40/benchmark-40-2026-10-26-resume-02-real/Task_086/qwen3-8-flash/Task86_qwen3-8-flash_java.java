import java.io.*;
import java.net.*;
import java.nio.file.*;
import java.security.*;
import java.util.*;

public class Main {
    public static void main(String[] args) {
        if (args.length < 1) {
            System.out.println("Usage: java Main <https-url> [expected-sha256]");
            return;
        }

        Path tempFile = null;
        Scanner scanner = new Scanner(System.in);

        try {
            tempFile = Files.createTempFile("download-", ".exe");
            download(args[0], tempFile);

            if (args.length >= 2) {
                String expected = args[1].trim().toLowerCase();
                String actual = sha256(tempFile);
                if (!expected.equals(actual)) {
                    throw new SecurityException("SHA-256 verification failed. Expected: " + expected + ", actual: " + actual);
                }
            }

            System.out.print("Execute downloaded EXE? (type YES to confirm): ");
            if (!scanner.hasNextLine()) {
                System.out.println("