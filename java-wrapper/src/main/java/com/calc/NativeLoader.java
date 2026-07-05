package com.calc;

import java.io.*;
import java.nio.file.*;

/**
 * Loads the native calculation engine library.
 * Extracts the appropriate native library from the JAR at runtime
 * based on the current operating system and architecture.
 *
 * JAR resource layout:
 *   /jni/<os>/<arch>/libscicalc-native.<ext>
 * e.g. /jni/linux/x86_64/libscicalc-native.so
 */
public class NativeLoader {
    private static boolean loaded = false;

    /**
     * Load the native library if not already loaded.
     */
    public static synchronized void load() {
        if (loaded) return;

        try {
            String os = osName();
            String arch = archName();
            String libFile = libFileName(os);

            // Resource path inside the JAR: /jni/<os>/<arch>/<libFile>
            String resourcePath = "/jni/" + os + "/" + arch + "/" + libFile;

            InputStream is = NativeLoader.class.getResourceAsStream(resourcePath);
            if (is == null) {
                // Also try classloader path (without leading /)
                is = NativeLoader.class.getClassLoader().getResourceAsStream(
                    "jni/" + os + "/" + arch + "/" + libFile);
            }

            if (is != null) {
                // Extract to a temp file and load
                File tempDir = new File(System.getProperty("java.io.tmpdir"), "scicalc-native");
                tempDir.mkdirs();
                File libFileOnDisk = new File(tempDir, libFile);
                libFileOnDisk.deleteOnExit();
                Files.copy(is, libFileOnDisk.toPath(), StandardCopyOption.REPLACE_EXISTING);
                is.close();
                System.load(libFileOnDisk.getAbsolutePath());
                loaded = true;
            } else {
                // Fallback: try system library path
                System.loadLibrary("scicalc-native");
                loaded = true;
            }
        } catch (IOException e) {
            throw new RuntimeException("Failed to load native library: " + e.getMessage(), e);
        } catch (UnsatisfiedLinkError e) {
            throw new RuntimeException("Native library not found: " + e.getMessage(), e);
        }
    }

    private static String osName() {
        String s = System.getProperty("os.name", "").toLowerCase();
        if (s.contains("linux"))   return "linux";
        if (s.contains("mac"))     return "macos";
        if (s.contains("windows")) return "windows";
        return s;
    }

    private static String archName() {
        String s = System.getProperty("os.arch", "").toLowerCase();
        if (s.equals("x86_64") || s.equals("amd64")) return "x86_64";
        if (s.equals("aarch64") || s.equals("arm64")) return "aarch64";
        return s;
    }

    private static String libFileName(String os) {
        switch (os) {
            case "windows": return "scicalc-native.dll";
            case "macos":   return "libscicalc-native.dylib";
            default:        return "libscicalc-native.so";
        }
    }
}
