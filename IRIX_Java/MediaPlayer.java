// MediaPlayer.java - Enhanced media player simulation
// Improvements: Playlist, volume control, repeat/shuffle modes, better visualization

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.util.List;

/**
 * A simulated media player with playlist and visualization.
 */
public class MediaPlayer extends JPanel {
    
    private JButton playButton, pauseButton, stopButton;
    private JButton prevButton, nextButton;
    private JSlider progressSlider, volumeSlider;
    private JLabel statusLabel, timeLabel;
    private JList<String> playlistView;
    private DefaultListModel<String> playlistModel;
    private Timer playbackTimer, visualizationTimer;
    private VisualizationPanel visualization;
    
    private int currentTime = 0;
    private int totalTime = 180; // 3 minutes default
    private boolean isPlaying = false;
    private boolean repeatMode = false;
    private boolean shuffleMode = false;
    private int currentTrack = 0;
    
    // Sample playlist
    private final List<String[]> tracks = Arrays.asList(
        new String[]{"Track 1 - Ambient Dreams", "3:24"},
        new String[]{"Track 2 - Digital Sunset", "4:12"},
        new String[]{"Track 3 - Silicon Valley", "2:58"},
        new String[]{"Track 4 - IRIX Theme", "3:45"},
        new String[]{"Track 5 - Graphics Pipeline", "5:01"}
    );
    
    public MediaPlayer() {
        setLayout(new BorderLayout(5, 5));
        setBorder(BorderFactory.createEmptyBorder(10, 10, 10, 10));
        
        initializeUI();
        initializeTimers();
        loadPlaylist();
    }
    
    private void initializeUI() {
        // Top - Now playing info
        JPanel infoPanel = new JPanel(new BorderLayout());
        statusLabel = new JLabel("Select a track to play", JLabel.CENTER);
        statusLabel.setFont(new Font("SansSerif", Font.BOLD, 14));
        infoPanel.add(statusLabel, BorderLayout.CENTER);
        
        add(infoPanel, BorderLayout.NORTH);
        
        // Center - Visualization and playlist
        JPanel centerPanel = new JPanel(new BorderLayout(5, 5));
        
        // Visualization
        visualization = new VisualizationPanel();
        visualization.setPreferredSize(new Dimension(300, 120));
        centerPanel.add(visualization, BorderLayout.CENTER);
        
        // Playlist
        playlistModel = new DefaultListModel<>();
        playlistView = new JList<>(playlistModel);
        playlistView.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        playlistView.setFont(new Font("SansSerif", Font.PLAIN, 12));
        playlistView.addMouseListener(new MouseAdapter() {
            @Override
            public void mouseClicked(MouseEvent e) {
                if (e.getClickCount() == 2) {
                    playSelectedTrack();
                }
            }
        });
        
        JScrollPane playlistScroll = new JScrollPane(playlistView);
        playlistScroll.setPreferredSize(new Dimension(300, 100));
        playlistScroll.setBorder(BorderFactory.createTitledBorder("Playlist"));
        centerPanel.add(playlistScroll, BorderLayout.SOUTH);
        
        add(centerPanel, BorderLayout.CENTER);
        
        // Bottom - Controls
        JPanel controlPanel = new JPanel(new BorderLayout(5, 5));
        
        // Progress and time
        JPanel progressPanel = new JPanel(new BorderLayout(5, 0));
        timeLabel = new JLabel("0:00 / 0:00");
        timeLabel.setFont(new Font("Monospaced", Font.PLAIN, 11));
        
        progressSlider = new JSlider(0, 100, 0);
        progressSlider.addChangeListener(e -> {
            if (progressSlider.getValueIsAdjusting()) {
                currentTime = (int)(progressSlider.getValue() * totalTime / 100.0);
                updateTimeLabel();
            }
        });
        
        progressPanel.add(progressSlider, BorderLayout.CENTER);
        progressPanel.add(timeLabel, BorderLayout.EAST);
        controlPanel.add(progressPanel, BorderLayout.NORTH);
        
        // Buttons
        JPanel buttonPanel = new JPanel(new FlowLayout(FlowLayout.CENTER, 5, 5));
        
        prevButton = createButton("\u23EE", "Previous");
        playButton = createButton("\u25B6", "Play");
        pauseButton = createButton("\u23F8", "Pause");
        stopButton = createButton("\u23F9", "Stop");
        nextButton = createButton("\u23ED", "Next");
        
        JToggleButton repeatButton = new JToggleButton("\uD83D\uDD01");
        repeatButton.setToolTipText("Repeat");
        repeatButton.addActionListener(e -> repeatMode = repeatButton.isSelected());
        
        JToggleButton shuffleButton = new JToggleButton("\uD83D\uDD00");
        shuffleButton.setToolTipText("Shuffle");
        shuffleButton.addActionListener(e -> shuffleMode = shuffleButton.isSelected());
        
        prevButton.addActionListener(e -> previousTrack());
        playButton.addActionListener(e -> play());
        pauseButton.addActionListener(e -> pause());
        stopButton.addActionListener(e -> stop());
        nextButton.addActionListener(e -> nextTrack());
        
        buttonPanel.add(shuffleButton);
        buttonPanel.add(prevButton);
        buttonPanel.add(playButton);
        buttonPanel.add(pauseButton);
        buttonPanel.add(stopButton);
        buttonPanel.add(nextButton);
        buttonPanel.add(repeatButton);
        
        // Volume
        buttonPanel.add(new JLabel(" Vol:"));
        volumeSlider = new JSlider(0, 100, 75);
        volumeSlider.setPreferredSize(new Dimension(80, 20));
        buttonPanel.add(volumeSlider);
        
        controlPanel.add(buttonPanel, BorderLayout.CENTER);
        add(controlPanel, BorderLayout.SOUTH);
    }
    
    private JButton createButton(String text, String tooltip) {
        JButton btn = new JButton(text);
        btn.setFont(new Font("SansSerif", Font.PLAIN, 16));
        btn.setToolTipText(tooltip);
        btn.setFocusPainted(false);
        return btn;
    }
    
    private void initializeTimers() {
        // Playback timer
        playbackTimer = new Timer(1000, e -> {
            if (isPlaying && currentTime < totalTime) {
                currentTime++;
                progressSlider.setValue((int)(currentTime * 100.0 / totalTime));
                updateTimeLabel();
            } else if (currentTime >= totalTime) {
                onTrackEnd();
            }
        });
        
        // Visualization timer
        visualizationTimer = new Timer(50, e -> {
            if (isPlaying) {
                visualization.repaint();
            }
        });
        visualizationTimer.start();
    }
    
    private void loadPlaylist() {
        for (String[] track : tracks) {
            playlistModel.addElement(track[0] + " [" + track[1] + "]");
        }
        playlistView.setSelectedIndex(0);
    }
    
    private void playSelectedTrack() {
        int selected = playlistView.getSelectedIndex();
        if (selected >= 0) {
            currentTrack = selected;
            loadTrack(currentTrack);
            play();
        }
    }
    
    private void loadTrack(int index) {
        if (index >= 0 && index < tracks.size()) {
            String[] track = tracks.get(index);
            statusLabel.setText("\uD83C\uDFB5 " + track[0]);
            
            // Parse duration
            String[] parts = track[1].split(":");
            totalTime = Integer.parseInt(parts[0]) * 60 + Integer.parseInt(parts[1]);
            currentTime = 0;
            progressSlider.setValue(0);
            updateTimeLabel();
            
            playlistView.setSelectedIndex(index);
        }
    }
    
    private void play() {
        if (currentTrack >= 0 && !isPlaying) {
            if (currentTime == 0) {
                loadTrack(currentTrack);
            }
            isPlaying = true;
            playbackTimer.start();
            statusLabel.setText("\uD83C\uDFB5 " + tracks.get(currentTrack)[0]);
        }
    }
    
    private void pause() {
        isPlaying = false;
        playbackTimer.stop();
    }
    
    private void stop() {
        isPlaying = false;
        playbackTimer.stop();
        currentTime = 0;
        progressSlider.setValue(0);
        updateTimeLabel();
        statusLabel.setText("Stopped");
    }
    
    private void previousTrack() {
        stop();
        currentTrack--;
        if (currentTrack < 0) {
            currentTrack = tracks.size() - 1;
        }
        loadTrack(currentTrack);
        play();
    }
    
    private void nextTrack() {
        stop();
        if (shuffleMode) {
            currentTrack = new Random().nextInt(tracks.size());
        } else {
            currentTrack++;
            if (currentTrack >= tracks.size()) {
                currentTrack = 0;
            }
        }
        loadTrack(currentTrack);
        play();
    }
    
    private void onTrackEnd() {
        if (repeatMode) {
            currentTime = 0;
            progressSlider.setValue(0);
        } else {
            nextTrack();
        }
    }
    
    private void updateTimeLabel() {
        timeLabel.setText(String.format("%d:%02d / %d:%02d",
            currentTime / 60, currentTime % 60,
            totalTime / 60, totalTime % 60));
    }
    
    // Visualization panel
    private class VisualizationPanel extends JPanel {
        private final Random random = new Random();
        private final int[] barHeights = new int[32];
        
        public VisualizationPanel() {
            setBackground(Color.BLACK);
        }
        
        @Override
        protected void paintComponent(Graphics g) {
            super.paintComponent(g);
            Graphics2D g2d = (Graphics2D) g;
            g2d.setRenderingHint(RenderingHints.KEY_ANTIALIASING, 
                                 RenderingHints.VALUE_ANTIALIAS_ON);
            
            int width = getWidth();
            int height = getHeight();
            int barWidth = width / barHeights.length - 2;
            
            // Update bar heights
            for (int i = 0; i < barHeights.length; i++) {
                if (isPlaying) {
                    int target = random.nextInt(height - 20) + 10;
                    barHeights[i] = barHeights[i] + (target - barHeights[i]) / 3;
                } else {
                    barHeights[i] = Math.max(5, barHeights[i] - 5);
                }
            }
            
            // Draw bars with gradient
            for (int i = 0; i < barHeights.length; i++) {
                int x = i * (barWidth + 2) + 2;
                int barHeight = barHeights[i];
                int y = height - barHeight;
                
                // Create gradient from green to red
                float ratio = (float) barHeight / height;
                Color color = new Color(
                    Math.min(255, (int)(ratio * 255)),
                    Math.min(255, (int)((1 - ratio * 0.5) * 255)),
                    50
                );
                
                g2d.setColor(color);
                g2d.fillRect(x, y, barWidth, barHeight);
                
                // Peak indicator
                g2d.setColor(Color.WHITE);
                g2d.fillRect(x, y - 2, barWidth, 2);
            }
            
            // Draw center line
            g2d.setColor(new Color(50, 50, 50));
            g2d.drawLine(0, height / 2, width, height / 2);
        }
    }
}
