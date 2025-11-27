// DrawingApp.java - Enhanced drawing application
// Improvements: Undo/redo support, fill mode, stroke width, better tool management

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.awt.geom.*;
import java.util.*;

/**
 * A simple vector drawing application inspired by classic paint programs.
 * Supports basic shapes, colors, and drawing operations.
 */
public class DrawingApp extends JPanel {
    
    // Drawing state
    private final java.util.List<DrawnShape> shapes;
    private final Stack<DrawnShape> undoStack;
    private Color currentColor = Color.BLACK;
    private String currentTool = "line";
    private int strokeWidth = 2;
    private boolean fillMode = false;
    
    // Current drawing operation
    private Point startPoint;
    private Shape currentShape;
    
    // UI Components
    private JToolBar toolbar;
    private JLabel statusLabel;
    
    public DrawingApp() {
        shapes = new ArrayList<>();
        undoStack = new Stack<>();
        
        setLayout(new BorderLayout());
        setBackground(Color.WHITE);
        
        // Create canvas panel
        JPanel canvas = createCanvas();
        toolbar = createToolbar();
        statusLabel = new JLabel(" Ready");
        
        add(toolbar, BorderLayout.NORTH);
        add(canvas, BorderLayout.CENTER);
        add(statusLabel, BorderLayout.SOUTH);
    }
    
    private JPanel createCanvas() {
        JPanel canvas = new JPanel() {
            @Override
            protected void paintComponent(Graphics g) {
                super.paintComponent(g);
                Graphics2D g2d = (Graphics2D) g;
                g2d.setRenderingHint(RenderingHints.KEY_ANTIALIASING, 
                                     RenderingHints.VALUE_ANTIALIAS_ON);
                
                // Draw all completed shapes
                for (DrawnShape ds : shapes) {
                    ds.draw(g2d);
                }
                
                // Draw current shape being created
                if (currentShape != null) {
                    g2d.setColor(currentColor);
                    g2d.setStroke(new BasicStroke(strokeWidth, 
                        BasicStroke.CAP_ROUND, BasicStroke.JOIN_ROUND));
                    if (fillMode) {
                        g2d.fill(currentShape);
                    }
                    g2d.draw(currentShape);
                }
            }
        };
        
        canvas.setBackground(Color.WHITE);
        setupMouseListeners(canvas);
        
        return canvas;
    }
    
    private void setupMouseListeners(JPanel canvas) {
        canvas.addMouseListener(new MouseAdapter() {
            @Override
            public void mousePressed(MouseEvent e) {
                startPoint = e.getPoint();
                currentShape = createShape(startPoint, startPoint);
                updateStatus("Drawing " + currentTool + "...");
            }
            
            @Override
            public void mouseReleased(MouseEvent e) {
                if (currentShape != null) {
                    // Save shape with its properties
                    shapes.add(new DrawnShape(currentShape, currentColor, 
                                              strokeWidth, fillMode));
                    undoStack.clear(); // Clear redo stack on new action
                    currentShape = null;
                    canvas.repaint();
                    updateStatus("Shape added. Total shapes: " + shapes.size());
                }
            }
        });
        
        canvas.addMouseMotionListener(new MouseMotionAdapter() {
            @Override
            public void mouseDragged(MouseEvent e) {
                if (startPoint != null) {
                    Point currentPoint = e.getPoint();
                    currentShape = createShape(startPoint, currentPoint);
                    canvas.repaint();
                }
            }
            
            @Override
            public void mouseMoved(MouseEvent e) {
                updateStatus(String.format("Position: (%d, %d) | Tool: %s | Color: %s", 
                    e.getX(), e.getY(), currentTool, getColorName(currentColor)));
            }
        });
    }
    
    private Shape createShape(Point start, Point end) {
        int x = Math.min(start.x, end.x);
        int y = Math.min(start.y, end.y);
        int width = Math.abs(end.x - start.x);
        int height = Math.abs(end.y - start.y);
        
        switch (currentTool) {
            case "line":
                return new Line2D.Double(start, end);
            case "rectangle":
                return new Rectangle2D.Double(x, y, width, height);
            case "ellipse":
                return new Ellipse2D.Double(x, y, width, height);
            case "roundrect":
                return new RoundRectangle2D.Double(x, y, width, height, 20, 20);
            default:
                return new Line2D.Double(start, end);
        }
    }
    
    private JToolBar createToolbar() {
        JToolBar tb = new JToolBar();
        tb.setFloatable(false);
        
        // Tool buttons
        String[] tools = {"line", "rectangle", "ellipse", "roundrect"};
        String[] toolLabels = {"Line", "Rectangle", "Ellipse", "Round Rect"};
        
        ButtonGroup toolGroup = new ButtonGroup();
        for (int i = 0; i < tools.length; i++) {
            JToggleButton btn = createToolButton(toolLabels[i], tools[i]);
            toolGroup.add(btn);
            tb.add(btn);
            if (i == 0) btn.setSelected(true);
        }
        
        tb.addSeparator();
        
        // Color palette
        Color[] colors = {
            Color.BLACK, Color.DARK_GRAY, Color.GRAY,
            Color.RED, new Color(255, 100, 100),
            Color.ORANGE, Color.YELLOW,
            Color.GREEN, new Color(100, 200, 100),
            Color.CYAN, Color.BLUE, new Color(100, 100, 255),
            Color.MAGENTA, new Color(200, 100, 200),
            Color.WHITE
        };
        
        for (Color color : colors) {
            tb.add(createColorButton(color));
        }
        
        JButton customColor = new JButton("...");
        customColor.setToolTipText("Choose custom color");
        customColor.addActionListener(e -> {
            Color chosen = JColorChooser.showDialog(this, "Choose Color", currentColor);
            if (chosen != null) {
                currentColor = chosen;
                updateStatus("Color changed to custom");
            }
        });
        tb.add(customColor);
        
        tb.addSeparator();
        
        // Fill mode toggle
        JToggleButton fillBtn = new JToggleButton("Fill");
        fillBtn.setToolTipText("Toggle fill mode");
        fillBtn.addActionListener(e -> {
            fillMode = fillBtn.isSelected();
            updateStatus("Fill mode: " + (fillMode ? "ON" : "OFF"));
        });
        tb.add(fillBtn);
        
        // Stroke width spinner
        tb.add(new JLabel(" Width: "));
        SpinnerNumberModel strokeModel = new SpinnerNumberModel(2, 1, 20, 1);
        JSpinner strokeSpinner = new JSpinner(strokeModel);
        strokeSpinner.setMaximumSize(new Dimension(50, 25));
        strokeSpinner.addChangeListener(e -> {
            strokeWidth = (Integer) strokeSpinner.getValue();
            updateStatus("Stroke width: " + strokeWidth);
        });
        tb.add(strokeSpinner);
        
        tb.addSeparator();
        
        // Action buttons
        JButton undoBtn = new JButton("Undo");
        undoBtn.addActionListener(e -> undo());
        tb.add(undoBtn);
        
        JButton redoBtn = new JButton("Redo");
        redoBtn.addActionListener(e -> redo());
        tb.add(redoBtn);
        
        JButton clearBtn = new JButton("Clear");
        clearBtn.addActionListener(e -> clear());
        tb.add(clearBtn);
        
        return tb;
    }
    
    private JToggleButton createToolButton(String label, String tool) {
        JToggleButton btn = new JToggleButton(label);
        btn.addActionListener(e -> {
            currentTool = tool;
            updateStatus("Tool: " + label);
        });
        return btn;
    }
    
    private JButton createColorButton(Color color) {
        JButton btn = new JButton() {
            @Override
            protected void paintComponent(Graphics g) {
                super.paintComponent(g);
                g.setColor(color);
                g.fillRect(4, 4, getWidth() - 8, getHeight() - 8);
                g.setColor(Color.BLACK);
                g.drawRect(4, 4, getWidth() - 9, getHeight() - 9);
            }
        };
        btn.setPreferredSize(new Dimension(25, 25));
        btn.setToolTipText(getColorName(color));
        btn.addActionListener(e -> {
            currentColor = color;
            updateStatus("Color: " + getColorName(color));
        });
        return btn;
    }
    
    private void undo() {
        if (!shapes.isEmpty()) {
            DrawnShape removed = shapes.remove(shapes.size() - 1);
            undoStack.push(removed);
            repaint();
            updateStatus("Undo. Shapes remaining: " + shapes.size());
        } else {
            updateStatus("Nothing to undo");
        }
    }
    
    private void redo() {
        if (!undoStack.isEmpty()) {
            DrawnShape restored = undoStack.pop();
            shapes.add(restored);
            repaint();
            updateStatus("Redo. Shapes: " + shapes.size());
        } else {
            updateStatus("Nothing to redo");
        }
    }
    
    private void clear() {
        int result = JOptionPane.showConfirmDialog(this, 
            "Clear all shapes?", "Confirm Clear", JOptionPane.YES_NO_OPTION);
        if (result == JOptionPane.YES_OPTION) {
            shapes.clear();
            undoStack.clear();
            repaint();
            updateStatus("Canvas cleared");
        }
    }
    
    private void updateStatus(String message) {
        if (statusLabel != null) {
            statusLabel.setText(" " + message);
        }
    }
    
    private String getColorName(Color c) {
        if (c.equals(Color.BLACK)) return "Black";
        if (c.equals(Color.WHITE)) return "White";
        if (c.equals(Color.RED)) return "Red";
        if (c.equals(Color.GREEN)) return "Green";
        if (c.equals(Color.BLUE)) return "Blue";
        if (c.equals(Color.YELLOW)) return "Yellow";
        if (c.equals(Color.CYAN)) return "Cyan";
        if (c.equals(Color.MAGENTA)) return "Magenta";
        if (c.equals(Color.ORANGE)) return "Orange";
        if (c.equals(Color.GRAY)) return "Gray";
        if (c.equals(Color.DARK_GRAY)) return "Dark Gray";
        return String.format("RGB(%d,%d,%d)", c.getRed(), c.getGreen(), c.getBlue());
    }
    
    /**
     * Represents a drawn shape with all its visual properties
     */
    private static class DrawnShape {
        final Shape shape;
        final Color color;
        final int strokeWidth;
        final boolean filled;
        
        DrawnShape(Shape shape, Color color, int strokeWidth, boolean filled) {
            this.shape = shape;
            this.color = color;
            this.strokeWidth = strokeWidth;
            this.filled = filled;
        }
        
        void draw(Graphics2D g2d) {
            g2d.setColor(color);
            g2d.setStroke(new BasicStroke(strokeWidth, 
                BasicStroke.CAP_ROUND, BasicStroke.JOIN_ROUND));
            if (filled) {
                g2d.fill(shape);
            }
            g2d.draw(shape);
        }
    }
}
