CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2 -pthread
TARGET = xenix_server
INSTALL_DIR = /usr/local/bin
SERVICE_DIR = /etc/systemd/system
STATE_DIR = /var/lib/xenix

all: $(TARGET)

$(TARGET): xenix_server.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) xenix_server.cpp

clean:
	rm -f $(TARGET)

install: $(TARGET)
	@echo "Installing XENIX Server..."
	# Create user and group if they don't exist
	@if ! id -u xenix > /dev/null 2>&1; then \
		sudo useradd -r -s /bin/false xenix; \
	fi
	# Create state directory
	sudo mkdir -p $(STATE_DIR)
	sudo chown xenix:xenix $(STATE_DIR)
	# Install binary
	sudo cp $(TARGET) $(INSTALL_DIR)/
	sudo chmod 755 $(INSTALL_DIR)/$(TARGET)
	# Install systemd service
	sudo cp xenix.service $(SERVICE_DIR)/
	sudo systemctl daemon-reload
	@echo ""
	@echo "Installation complete!"
	@echo ""
	@echo "To start the service:"
	@echo "  sudo systemctl start xenix"
	@echo ""
	@echo "To enable on boot:"
	@echo "  sudo systemctl enable xenix"
	@echo ""
	@echo "To check status:"
	@echo "  sudo systemctl status xenix"
	@echo ""
	@echo "To view logs:"
	@echo "  sudo journalctl -u xenix -f"
	@echo ""

uninstall:
	@echo "Uninstalling XENIX Server..."
	sudo systemctl stop xenix 2>/dev/null || true
	sudo systemctl disable xenix 2>/dev/null || true
	sudo rm -f $(INSTALL_DIR)/$(TARGET)
	sudo rm -f $(SERVICE_DIR)/xenix.service
	sudo systemctl daemon-reload
	@echo "Uninstall complete!"
	@echo "Note: User 'xenix' and data in $(STATE_DIR) were not removed."

start:
	sudo systemctl start xenix

stop:
	sudo systemctl stop xenix

restart:
	sudo systemctl restart xenix

status:
	sudo systemctl status xenix

logs:
	sudo journalctl -u xenix -f

.PHONY: all clean install uninstall start stop restart status logs
