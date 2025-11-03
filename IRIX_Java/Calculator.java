// Calculator.java
import javax.swing.*;
import java.awt.*;
import java.awt.event.*;

public class Calculator extends JPanel {
    private JTextField display;
    private double firstNumber = 0;
    private String operator = "";
    private boolean startNewNumber = true;
    
    public Calculator() {
        setLayout(new BorderLayout(5, 5));
        setBorder(BorderFactory.createEmptyBorder(10, 10, 10, 10));
        
        // Display
        display = new JTextField("0");
        display.setEditable(false);
        display.setHorizontalAlignment(JTextField.RIGHT);
        display.setFont(new Font("Monospaced", Font.BOLD, 24));
        display.setBackground(Color.WHITE);
        display.setBorder(BorderFactory.createLineBorder(Color.GRAY, 2));
        
        add(display, BorderLayout.NORTH);
        
        // Button panel
        JPanel buttonPanel = new JPanel(new GridLayout(5, 4, 5, 5));
        
        String[][] buttons = {
            {"7", "8", "9", "/"},
            {"4", "5", "6", "*"},
            {"1", "2", "3", "-"},
            {"0", ".", "=", "+"},
            {"C", "CE", "√", "±"}
        };
        
        for (String[] row : buttons) {
            for (String text : row) {
                JButton button = createButton(text);
                buttonPanel.add(button);
            }
        }
        
        add(buttonPanel, BorderLayout.CENTER);
    }
    
    private JButton createButton(String text) {
        JButton button = new JButton(text);
        button.setFont(new Font("Dialog", Font.BOLD, 18));
        button.setFocusPainted(false);
        
        // Set colors based on button type
        if (text.matches("[0-9.]")) {
            button.setBackground(new Color(230, 230, 230));
        } else if (text.matches("[+\\-*/=]")) {
            button.setBackground(new Color(255, 200, 100));
        } else {
            button.setBackground(new Color(200, 200, 255));
        }
        
        button.addActionListener(e -> handleButton(text));
        
        return button;
    }
    
    private void handleButton(String text) {
        switch (text) {
            case "0": case "1": case "2": case "3": case "4":
            case "5": case "6": case "7": case "8": case "9":
                handleNumber(text);
                break;
            case ".":
                handleDecimal();
                break;
            case "+": case "-": case "*": case "/":
                handleOperator(text);
                break;
            case "=":
                calculate();
                break;
            case "C":
                clear();
                break;
            case "CE":
                clearEntry();
                break;
            case "√":
                squareRoot();
                break;
            case "±":
                negate();
                break;
        }
    }
    
    private void handleNumber(String digit) {
        if (startNewNumber) {
            display.setText(digit);
            startNewNumber = false;
        } else {
            if (display.getText().equals("0")) {
                display.setText(digit);
            } else {
                display.setText(display.getText() + digit);
            }
        }
    }
    
    private void handleDecimal() {
        if (startNewNumber) {
            display.setText("0.");
            startNewNumber = false;
        } else if (!display.getText().contains(".")) {
            display.setText(display.getText() + ".");
        }
    }
    
    private void handleOperator(String op) {
        if (!operator.isEmpty() && !startNewNumber) {
            calculate();
        }
        firstNumber = Double.parseDouble(display.getText());
        operator = op;
        startNewNumber = true;
    }
    
    private void calculate() {
        if (operator.isEmpty()) return;
        
        double secondNumber = Double.parseDouble(display.getText());
        double result = 0;
        
        switch (operator) {
            case "+":
                result = firstNumber + secondNumber;
                break;
            case "-":
                result = firstNumber - secondNumber;
                break;
            case "*":
                result = firstNumber * secondNumber;
                break;
            case "/":
                if (secondNumber != 0) {
                    result = firstNumber / secondNumber;
                } else {
                    display.setText("Error");
                    operator = "";
                    startNewNumber = true;
                    return;
                }
                break;
        }
        
        display.setText(formatResult(result));
        operator = "";
        startNewNumber = true;
    }
    
    private void clear() {
        display.setText("0");
        firstNumber = 0;
        operator = "";
        startNewNumber = true;
    }
    
    private void clearEntry() {
        display.setText("0");
        startNewNumber = true;
    }
    
    private void squareRoot() {
        double number = Double.parseDouble(display.getText());
        if (number >= 0) {
            display.setText(formatResult(Math.sqrt(number)));
        } else {
            display.setText("Error");
        }
        startNewNumber = true;
    }
    
    private void negate() {
        double number = Double.parseDouble(display.getText());
        display.setText(formatResult(-number));
    }
    
    private String formatResult(double result) {
        // Remove trailing zeros and decimal point if not needed
        if (result == (long) result) {
            return String.format("%d", (long) result);
        } else {
            return String.format("%.8f", result).replaceAll("0*$", "").replaceAll("\\.$", "");
        }
    }
}
