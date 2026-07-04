package com.calc;

/**
 * JNI wrapper for the C++ calculation engine.
 * Provides all engine functionality through native method calls.
 */
public class CalcEngine {
    private long nativeHandle; // Pointer to native CalcEngine instance

    public CalcEngine() {
        NativeLoader.load();
        nativeHandle = nativeCreate();
    }

    /**
     * Evaluate an expression string.
     * @param input The expression to evaluate
     * @return CalcResult containing the result or error
     */
    public CalcResult evaluate(String input) {
        if (nativeHandle == 0) {
            return CalcResult.error("Engine not initialized");
        }
        return nativeEvaluate(nativeHandle, input);
    }

    /**
     * Set the number format mode.
     * @param mode Format mode: "scientific", "fixed", "significant", "floating"
     */
    public void setFormatMode(String mode) {
        if (nativeHandle != 0) {
            nativeSetFormatMode(nativeHandle, mode);
        }
    }

    /**
     * Set the number base for output.
     * @param base Base: 2, 8, 10, or 16
     */
    public void setBase(int base) {
        if (nativeHandle != 0) {
            nativeSetBase(nativeHandle, base);
        }
    }

    /**
     * Set the precision in bits.
     * @param precision Precision (minimum 32, maximum 4096)
     */
    public void setPrecision(int precision) {
        if (nativeHandle != 0) {
            nativeSetPrecision(nativeHandle, precision);
        }
    }

    /**
     * Reset the engine (clear all user definitions).
     */
    public void reset() {
        if (nativeHandle != 0) {
            nativeReset(nativeHandle);
        }
    }

    /**
     * Clean up native resources.
     */
    public void dispose() {
        if (nativeHandle != 0) {
            nativeDestroy(nativeHandle);
            nativeHandle = 0;
        }
    }

    @Override
    protected void finalize() throws Throwable {
        dispose();
        super.finalize();
    }

    // ==================== Native methods ====================

    private native long nativeCreate();
    private native void nativeDestroy(long handle);
    private native CalcResult nativeEvaluate(long handle, String input);
    private native void nativeSetFormatMode(long handle, String mode);
    private native void nativeSetBase(long handle, int base);
    private native void nativeSetPrecision(long handle, int precision);
    private native void nativeReset(long handle);
}
