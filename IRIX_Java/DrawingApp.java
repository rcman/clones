// DrawingApp.java
import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.awt.geom.*;
import java.util.*;

public class DrawingApp extends JPanel {
    private ArrayList<Shape> shapes;
    private ArrayList<Color> shapeColors;
    private Color currentColor = Color.BLACK;
    private String currentTool = "line";
    private Point startPoint, currentPoint;
    private Shape currentShape;
    
    public DrawingApp() {
        shapes = new ArrayList<>();
        shapeColors = new ArrayList<>();
        setBackground(Color.WHITE);
        
        setupMouseListeners();
        createToolbar();
    }
    
    private void setupMouseListeners() {
        addMouseListener(new MouseAdapter() {
            public void mousePressed(MouseEvent e) {
                startPoint = e.getPoint();
                currentPoint = startPoint;
                
                switch (currentTool) {
                    case "line":
                        currentShape = new Line2D.Double(startPoint, startPoint);
                        break;
                    case "rectangle":
                        currentShape = new Rectangle2D.Double(
                            startPoint.x, startPoint.y, 0, 0);
                        break;
                    case "ellipse":
                        currentShape = new Ellipse2D.Double(
                            startPoint.x, startPoint.y, 0, 0);
                        break;
                }
            }
            
            public void mouseReleased(MouseEvent e) {
                if (currentShape != null) {
                    shapes.add(currentShape);
                    shapeColors.add(currentColor);
                    currentShape = null;
                    repaint();
                }
            }
        });
        
        addMouseMotionListener(new MouseMotionAdapter() {
            public void mouseDragged(MouseEvent e) {
                if (currentShape != null) {
                    currentPoint = e.getPoint();
                    
                    switch (currentTool) {
                        case "line":
                            ((Line2D) currentShape).setLine(startPoint, currentPoint);
                            break;
                        case "rectangle":
                            int x = Math.min(startPoint.x, currentPoint.x);
                            int y = Math.min(startPoint.y, currentPoint.y);
                            int width = Math.abs(currentPoint.x - startPoint.x);
                            int height = Math.abs(currentPoint.y - startPoint.y);
                            ((Rectangle2D) currentShape).setFrame(x, y, width, height);
                            break;
                        case "ellipse":
                            x = Math.min(startPoint.x, currentPoint.x);
                            y = Math.min(startPoint.y, currentPoint.y);
                            width = Math.abs(currentPoint.x - startPoint.x);
                            height = Math.abs(currentPoint.y - startPoint.y);
                            ((Ellipse2D) currentShape).setFrame(x, y, width, height);
                            break;
                    }
                    repaint();
                }
            }
        });
    }
    
    private void createToolbar() {
        JToolBar toolbar = new JToolBar();
        toolbar.setFloatable(false);
        
        String[] tools = {"line", "rectangle", "ellipse", "clear"};
        Color[] colors = {Color.BLACK, Color.RED, Color.GREEN, Color.BLUE, 
                         Color.YELLOW, Color.MAGENTA, Color.CYAN};
        
        for (String tool : tools) {
            JButton btn = new JButton(tool.substring(0, 1).toUpperCase() + 
                                     tool.substring(1));
            btn.addActionListener(e -> currentTool = tool);
            toolbar.add(btn);
        }
        
        for (Color color : colors) {
            JButton colorBtn = new JButton();
            colorBtn.setBackground(color);
            colorBtn.setPreferredSize(new Dimension(25, 25));
            colorBtn.addActionListener(e -> currentColor = color);
            toolbar.add(colorBtn);
        }
        
        JButton clearBtn = new JButton("Clear");
        clearBtn.addActionListener(e -> {
            shapes.clear();
            shapeColors.clear();
            repaint();
        });
        toolbar.add(clearBtn);
        
        // Add toolbar to parent if possible
        Container parent = getParent();
        if (parent != null) {
            ((BorderLayout)parent.getLayout()).addLayoutComponent(toolbar, BorderLayout.NORTH);
            parent.add(toolbar, BorderLayout.NORTH);
        }
    }
    
    @Override
    protected void paintComponent(Graphics g) {
        super.paintComponent(g);
        Graphics2D g2d = (Graphics2D) g;
        g2d.setRenderingHint(RenderingHints.KEY_ANTIALIASING, 
                           RenderingHints.VALUE_ANTIALIAS_ON);
        
        // Draw existing shapes
        for (int i = 0; i < shapes.size(); i++) {
            g2d.setColor(shapeColors.get(i));
            g2d.draw(shapes.get(i));
        }
        
        // Draw current shape being drawn
        if (currentShape != null) {
            g2d.setColor(currentColor);
            g2d.draw(currentShape);
        }
    }
}