// Calculator.java - Enhanced calculator with keyboard support and memory
// Improvements: Keyboard input, memory functions, percentage, better error handling

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.text.DecimalFormat;

/**
 * A feature-rich calculator with memory functions and keyboard support.
 */
public class Calculator extends JPanel {
    
    // Calculator state
    private double firstNumber = 0;
    private double memory = 0;
    private String operator = "";
    private boolean startNewNumber = true;
    private boolean hasError = false;
    
    // UI Components
    private JTextField display;
    private JLabel memoryIndicator;
    
    // Formatting
    private static final DecimalFormat DECIMAL_FORMAT = new DecimalFormat("#.##########");
    
    public Calculator() {
        setLayout(new BorderLayout(5, 5));
        setBorder(BorderFactory.createEmptyBorder(10, 10, 10, 10));
        
        initializeUI();
        setupKeyboardInput();
    }
    
    private void initializeUI() {
        // Top panel with memory indicator and display
        JPanel topPanel = new JPanel(new BorderLayout(5, 5));
        
        memoryIndicator = new JLabel(" ");
        memoryIndicator.setFont(new Font("Monospaced", Font.PLAIN, 10));
        memoryIndicator.setPreferredSize(new Dimension(0, 15));
        
        display = new JTextField("0");
        display.setEditable(false);
        display.setHorizontalAlignment(JTextField.RIGHT);
        display.setFont(new Font("Monospaced", Font.BOLD, 28));
        display.setBackground(Color.WHITE);
        display.setBorder(BorderFactory.createCompoundBorder(
            BorderFactory.createLineBorder(Color.GRAY, 2),
            BorderFactory.createEmptyBorder(5, 5, 5, 5)
        ));
        
        topPanel.add(memoryIndicator, BorderLayout.NORTH);
        topPanel.add(display, BorderLayout.CENTER);
        
        add(topPanel, BorderLayout.NORTH);
        
        // Button panel with grid layout
        JPanel buttonPanel = new JPanel(new GridLayout(6, 4, 5, 5));
        
        // Button definitions: [text, type]
        // Types: num=number, op=operator, func=function, mem=memory
        String[][] buttons = {
            {"MC", "mem"}, {"MR", "mem"}, {"M+", "mem"}, {"M-", "mem"},
            {"C", "func"},  {"CE", "func"}, {"%", "func"}, {"/", "op"},
            {"7", "num"},   {"8", "num"},   {"9", "num"},  {"*", "op"},
            {"4", "num"},   {"5", "num"},   {"6", "num"},  {"-", "op"},
            {"1", "num"},   {"2", "num"},   {"3", "num"},  {"+", "op"},
            {"\u00B1", "func"}, {"0", "num"}, {".", "num"}, {"=", "op"}
        };
        
        for (String[] btnDef : buttons) {
            buttonPanel.add(createButton(btnDef[0], btnDef[1]));
        }
        
        add(buttonPanel, BorderLayout.CENTER);
    }
    
    private JButton createButton(String text, String type) {
        JButton button = new JButton(text);
        button.setFont(new Font("Dialog", Font.BOLD, 18));
        button.setFocusPainted(false);
        button.setMargin(new Insets(10, 10, 10, 10));
        
        // Style based on button type
        switch (type) {
            case "num":
                button.setBackground(new Color(240, 240, 240));
                break;
            case "op":
                button.setBackground(new Color(255, 200, 100));
                break;
            case "func":
                button.setBackground(new Color(200, 220, 255));
                break;
            case "mem":
                button.setBackground(new Color(220, 255, 220));
                break;
        }
        
        button.addActionListener(e -> handleButton(text));
        
        return button;
    }
    
    private void setupKeyboardInput() {
        // Make panel focusable
        setFocusable(true);
        
        addKeyListener(new KeyAdapter() {
            @Override
            public void keyPressed(KeyEvent e) {
                char c = e.getKeyChar();
                int code = e.getKeyCode();
                
                if (Character.isDigit(c)) {
                    handleButton(String.valueOf(c));
                } else if (c == '.') {
                    handleButton(".");
                } else if (c == '+') {
                    handleButton("+");
                } else if (c == '-') {
                    handleButton("-");
                } else if (c == '*') {
                    handleButton("*");
                } else if (c == '/') {
                    handleButton("/");
                } else if (c == '%') {
                    handleButton("%");
                } else if (c == '\n' || c == '=') {
                    handleButton("=");
                } else if (code == KeyEvent.VK_BACK_SPACE) {
                    backspace();
                } else if (code == KeyEvent.VK_ESCAPE) {
                    handleButton("C");
                } else if (code == KeyEvent.VK_DELETE) {
                    handleButton("CE");
                }
            }
        });
        
        // Request focus when added to container
        addHierarchyListener(e -> {
            if ((e.getChangeFlags() & HierarchyEvent.SHOWING_CHANGED) != 0 && isShowing()) {
                requestFocusInWindow();
            }
        });
    }
    
    private void handleButton(String text) {
        // Clear error state on any input
        if (hasError && !text.equals("C") && !text.equals("CE")) {
            clear();
        }
        
        switch (text) {
            // Numbers
            case "0": case "1": case "2": case "3": case "4":
            case "5": case "6": case "7": case "8": case "9":
                handleNumber(text);
                break;
            
            // Decimal point
            case ".":
                handleDecimal();
                break;
            
            // Operators
            case "+": case "-": case "*": case "/":
                handleOperator(text);
                break;
            
            // Equals
            case "=":
                calculate();
                break;
            
            // Functions
            case "C":
                clear();
                break;
            case "CE":
                clearEntry();
                break;
            case "%":
                percentage();
                break;
            case "\u00B1":
                negate();
                break;
            
            // Memory
            case "MC":
                memoryClear();
                break;
            case "MR":
                memoryRecall();
                break;
            case "M+":
                memoryAdd();
                break;
            case "M-":
                memorySubtract();
                break;
        }
        
        // Keep focus for keyboard input
        requestFocusInWindow();
    }
    
    private void handleNumber(String digit) {
        if (startNewNumber) {
            display.setText(digit);
            startNewNumber = false;
        } else {
            String current = display.getText();
            // Prevent leading zeros (except for decimals)
            if (current.equals("0") && !digit.equals("0")) {
                display.setText(digit);
            } else if (!current.equals("0")) {
                display.setText(current + digit);
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
        firstNumber = parseDisplay();
        operator = op;
        startNewNumber = true;
    }
    
    private void calculate() {
        if (operator.isEmpty()) return;
        
        double secondNumber = parseDisplay();
        double result = 0;
        boolean error = false;
        
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
                    showError("Cannot divide by zero");
                    error = true;
                }
                break;
        }
        
        if (!error) {
            displayResult(result);
        }
        
        operator = "";
        startNewNumber = true;
    }
    
    private void clear() {
        display.setText("0");
        firstNumber = 0;
        operator = "";
        startNewNumber = true;
        hasError = false;
    }
    
    private void clearEntry() {
        display.setText("0");
        startNewNumber = true;
        hasError = false;
    }
    
    private void percentage() {
        double value = parseDisplay();
        if (!operator.isEmpty()) {
            // If operator pending, calculate percentage of first number
            value = firstNumber * (value / 100);
        } else {
            // Otherwise just divide by 100
            value = value / 100;
        }
        displayResult(value);
        startNewNumber = true;
    }
    
    private void negate() {
        if (!display.getText().equals("0")) {
            double value = parseDisplay();
            displayResult(-value);
        }
    }
    
    private void backspace() {
        if (!startNewNumber && !hasError) {
            String current = display.getText();
            if (current.length() > 1) {
                // Handle negative numbers
                if (current.length() == 2 && current.startsWith("-")) {
                    display.setText("0");
                    startNewNumber = true;
                } else {
                    display.setText(current.substring(0, current.length() - 1));
                }
            } else {
                display.setText("0");
                startNewNumber = true;
            }
        }
    }
    
    // Memory functions
    
    private void memoryClear() {
        memory = 0;
        updateMemoryIndicator();
    }
    
    private void memoryRecall() {
        displayResult(memory);
        startNewNumber = true;
    }
    
    private void memoryAdd() {
        memory += parseDisplay();
        updateMemoryIndicator();
        startNewNumber = true;
    }
    
    private void memorySubtract() {
        memory -= parseDisplay();
        updateMemoryIndicator();
        startNewNumber = true;
    }
    
    private void updateMemoryIndicator() {
        if (memory != 0) {
            memoryIndicator.setText("M");
        } else {
            memoryIndicator.setText(" ");
        }
    }
    
    // Helper methods
    
    private double parseDisplay() {
        try {
            return Double.parseDouble(display.getText());
        } catch (NumberFormatException e) {
            return 0;
        }
    }
    
    private void displayResult(double result) {
        // Handle special values
        if (Double.isNaN(result)) {
            showError("Error");
            return;
        }
        if (Double.isInfinite(result)) {
            showError("Overflow");
            return;
        }
        
        // Format result nicely
        String formatted = DECIMAL_FORMAT.format(result);
        
        // Limit display length
        if (formatted.length() > 15) {
            // Use scientific notation for very large/small numbers
            formatted = String.format("%.8E", result);
        }
        
        display.setText(formatted);
    }
    
    private void showError(String message) {
        display.setText(message);
        hasError = true;
        operator = "";
        startNewNumber = true;
    }
}
