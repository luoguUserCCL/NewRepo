package com.calc;

import java.io.*;
import java.nio.file.*;

/**
 * Loads the native calculation engine library.
 * Extracts the appropriate native library from the JAR at runtime
 * based on the current operating system and architecture.
 */
public class NativeLoader {
    private static boolean loaded = false;

    /**
     * Load the native library if not already loaded.
     */
    public static synchronized void load() {
        if (loaded) return;

        try {
            String libName = getNativeLibraryName();
            String resourcePath = "native/" + libName;

            // Try to extract from JAR
            InputStream is = NativeLoader.class.getClassLoader()
                .getResourceAsStream(resourcePath);

            if (is != null) {
                // Extract to temp directory
                File tempDir = new File(System.getProperty("java.io.tmpdir"), "calc-native");
                tempDir.mkdirs();
                File libFile = new File(tempDir, libName);

                // Delete on exit
                libFile.deleteOnExit();

                // Copy library to temp file
                Files.copy(is, libFile.toPath(), StandardCopyOption.REPLACE_EXISTING);
                is.close();

                // Load the library
                System.load(libFile.getAbsolutePath());
                loaded = true;
            } else {
                // Fallback: try system library path
                System.loadLibrary("calc-bridge");
                loaded = true;
            }
        } catch (IOException e) {
            throw new RuntimeException("Failed to load native library: " + e.getMessage(), e);
        } catch (UnsatisfiedLinkError e) {
            throw new RuntimeException("Native library not found: " + e.getMessage(), e);
        }
    }

    /**
     * Determine the native library filename for the current platform.
     */
    private static String getNativeLibraryName() {
        String os = System.getProperty("os.name").toLowerCase();
        String arch = System.getProperty("os.arch").toLowerCase();

        String osPart;
        if (os.contains("linux")) {
            osPart = "linux";
        } else if (os.contains("windows")) {
            osPart = "windows";
        } else if (os.contains("mac")) {
            osPart = "macos";
        } else {
            osPart = "unknown";
        }

        String archPart;
        if (arch.contains("x86_64") || arch.contains("amd64")) {
            archPart = "x86_64";
        } else if (arch.contains("aarch64") || arch.contains("arm64")) {
            archPart = "aarch64";
        } else {
            archPart = arch;
        }

        String ext;
        if (os.contains("windows")) {
            ext = ".dll";
        } else if (os.contains("mac")) {
            ext = ".dylib";
        } else {
            ext = ".so";
        }

        return "calc-bridge-" + osPart + "-" + archPart + ext;
    }
}
