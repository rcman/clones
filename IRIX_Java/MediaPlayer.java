// MediaPlayer.java
import javax.swing.*;
import java.awt.*;
import java.awt.event.*;

public class MediaPlayer extends JPanel {
    private JButton playButton, pauseButton, stopButton;
    private JSlider progressSlider;
    private JLabel statusLabel;
    private Timer playbackTimer;
    private int currentTime = 0;
    private boolean isPlaying = false;
    
    public MediaPlayer() {
        setLayout(new BorderLayout());
        
        // Status display
        statusLabel = new JLabel("Stopped", JLabel.CENTER);
        statusLabel.setFont(new Font("Arial", Font.BOLD, 14));
        
        // Control buttons
        JPanel controlPanel = new JPanel(new FlowLayout());
        
        playButton = new JButton("▶");
        pauseButton = new JButton("❚❚");
        stopButton = new JButton("■");
        
        playButton.setFont(new Font("Arial", Font.BOLD, 16));
        pauseButton.setFont(new Font("Arial", Font.BOLD, 16));
        stopButton.setFont(new Font("Arial", Font.BOLD, 16));
        
        playButton.addActionListener(e -> startPlayback());
        pauseButton.addActionListener(e -> pausePlayback());
        stopButton.addActionListener(e -> stopPlayback());
        
        controlPanel.add(playButton);
        controlPanel.add(pauseButton);
        controlPanel.add(stopButton);
        
        // Progress slider
        progressSlider = new JSlider(0, 100, 0);
        progressSlider.addChangeListener(e -> {
            if (!isPlaying) {
                currentTime = progressSlider.getValue();
                updateStatus();
            }
        });
        
        // Visualization panel (simulated)
        JPanel visualization = new JPanel() {
            @Override
            protected void paintComponent(Graphics g) {
                super.paintComponent(g);
                Graphics2D g2d = (Graphics2D) g;
                
                // Draw audio visualization
                int width = getWidth();
                int height = getHeight();
                int centerY = height / 2;
                
                g2d.setColor(new Color(0, 102, 204));
                for (int i = 0; i < width; i += 4) {
                    if (isPlaying) {
                        int barHeight = (int) (Math.random() * height / 2);
                        g2d.fillRect(i, centerY - barHeight, 3, barHeight * 2);
                    }
                }
                
                // Draw progress indicator
                if (isPlaying) {
                    int progressX = (int) (width * (currentTime / 100.0));
                    g2d.setColor(Color.RED);
                    g2d.drawLine(progressX, 0, progressX, height);
                }
            }
        };
        visualization.setPreferredSize(new Dimension(300, 100));
        visualization.setBackground(Color.BLACK);
        
        // Animation timer for visualization
        Timer vizTimer = new Timer(100, e -> visualization.repaint());
        vizTimer.start();
        
        // Playback simulation timer
        playbackTimer = new Timer(100, e -> {
            if (isPlaying && currentTime < 100) {
                currentTime++;
                progressSlider.setValue(currentTime);
                updateStatus();
            } else if (currentTime >= 100) {
                stopPlayback();
            }
        });
        
        add(statusLabel, BorderLayout.NORTH);
        add(visualization, BorderLayout.CENTER);
        add(progressSlider, BorderLayout.SOUTH);
        add(controlPanel, BorderLayout.SOUTH);
        
        updateStatus();
    }
    
    private void startPlayback() {
        isPlaying = true;
        playbackTimer.start();
        updateStatus();
    }
    
    private void pausePlayback() {
        isPlaying = false;
        playbackTimer.stop();
        updateStatus();
    }
    
    private void stopPlayback() {
        isPlaying = false;
        playbackTimer.stop();
        currentTime = 0;
        progressSlider.setValue(0);
        updateStatus();
    }
    
    private void updateStatus() {
        String status = isPlaying ? "Playing" : "Stopped";
        statusLabel.setText(status + " - " + currentTime + "%");
    }
}