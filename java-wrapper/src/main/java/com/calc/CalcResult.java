package com.calc;

/**
 * Result of a calculation.
 * Contains either a successful value or an error message.
 */
public class CalcResult {
    private final String value;
    private final String error;
    private final boolean isError;

    private CalcResult(String value, String error, boolean isError) {
        this.value = value;
        this.error = error;
        this.isError = isError;
    }

    /**
     * Create a successful result.
     */
    public static CalcResult success(String value) {
        return new CalcResult(value, null, false);
    }

    /**
     * Create an error result.
     */
    public static CalcResult error(String error) {
        return new CalcResult(null, error, true);
    }

    /**
     * Get the result value (null if error).
     */
    public String getValue() {
        return value;
    }

    /**
     * Get the error message (null if success).
     */
    public String getError() {
        return error;
    }

    /**
     * Check if this is an error result.
     */
    public boolean hasError() {
        return isError;
    }

    @Override
    public String toString() {
        if (isError) {
            return "Error: " + error;
        }
        return value;
    }
}
