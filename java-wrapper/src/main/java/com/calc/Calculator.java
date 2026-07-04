package com.calc;

/**
 * Scientific Calculator — Java wrapper entry point.
 * Loads native library and provides CLI interface.
 */
public class Calculator {
    public static void main(String[] args) {
        System.out.println("Scientific Calculator v1.0.0");
        System.out.println("Powered by C++ high-precision engine via JNI");

        try {
            CalcEngine engine = new CalcEngine();

            if (args.length > 0) {
                // Evaluate expression from command line
                String input = String.join(" ", args);
                CalcResult result = engine.evaluate(input);
                if (result.hasError()) {
                    System.err.println("Error: " + result.getError());
                } else {
                    System.out.println(input + " = " + result.getValue());
                }
            } else {
                // Interactive mode
                System.out.println("Enter expressions to evaluate (type 'quit' to exit):");
                java.util.Scanner scanner = new java.util.Scanner(System.in);
                while (true) {
                    System.out.print(">> ");
                    String line = scanner.nextLine().trim();
                    if (line.equalsIgnoreCase("quit") || line.equalsIgnoreCase("exit")) {
                        break;
                    }
                    if (line.isEmpty()) continue;

                    CalcResult result = engine.evaluate(line);
                    if (result.hasError()) {
                        System.err.println("Error: " + result.getError());
                    } else {
                        System.out.println("= " + result.getValue());
                    }
                }
                scanner.close();
            }
        } catch (Exception e) {
            System.err.println("Fatal error: " + e.getMessage());
            e.printStackTrace();
            System.exit(1);
        }
    }
}
