# Build variables
CXX := g++
CXXFLAGS := -std=c++17 -Wall -O2
TARGET := kubsh
SRC_DIR := src
BUILD_DIR := build
DEB_DIR := deb_package
INSTALL_DIR := $(DEB_DIR)/usr/bin
DEB_NAME := kubsh_1.0-1_amd64.deb

# Default target
all: build

# Build the program
build: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET): $(SRC_DIR)/main.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@
	@echo "Compiled: $@"

# Create deb package structure
prepare-deb:
	@echo "Preparing deb package structure..."
	@mkdir -p $(DEB_DIR)/DEBIAN
	@mkdir -p $(INSTALL_DIR)
	@mkdir -p $(DEB_DIR)/usr/share/doc/kubsh
	@mkdir -p $(DEB_DIR)/usr/share/man/man1

# Copy binary to package directory
install: build prepare-deb
	@echo "Installing binary to package directory..."
	cp $(BUILD_DIR)/$(TARGET) $(INSTALL_DIR)/$(TARGET)
	chmod 755 $(INSTALL_DIR)/$(TARGET)

# Create man page (опционально)
$(DEB_DIR)/usr/share/man/man1/kubsh.1:
	@echo "Creating manual page..."
	@echo '.TH KUBSH 1 "$(shell date +"%B %Y")" "kubsh 1.0" "User Commands"' > $@
	@echo '.SH NAME' >> $@
	@echo 'kubsh \- Kubernetes Shell' >> $@
	@echo '.SH SYNOPSIS' >> $@
	@echo '.B kubsh' >> $@
	@echo '[OPTIONS]' >> $@
	@echo '.SH DESCRIPTION' >> $@
	@echo 'kubsh is a custom shell for Kubernetes operations.' >> $@
	@echo '.SH OPTIONS' >> $@
	@echo 'None yet.' >> $@
	@echo '.SH AUTHOR' >> $@
	@echo 'Your Name <your.email@example.com>' >> $@
	@gzip -f $@

# Create control file for deb package
$(DEB_DIR)/DEBIAN/control:
	@echo "Creating control file..."
	@echo "Package: kubsh" > $@
	@echo "Version: 1.0" >> $@
	@echo "Section: utils" >> $@
	@echo "Priority: optional" >> $@
	@echo "Architecture: amd64" >> $@
	@echo "Maintainer: Your Name <your.email@example.com>" >> $@
	@echo "Description: Kubernetes Shell" >> $@
	@echo " kubsh is a custom shell designed for Kubernetes operations." >> $@
	@echo " It provides convenient commands for working with k8s clusters." >> $@

# Create post-installation script (опционально)
$(DEB_DIR)/DEBIAN/postinst:
	@echo "#!/bin/bash" > $@
	@echo "# Post-installation script for kubsh" >> $@
	@echo "echo 'kubsh has been installed successfully!'" >> $@
	@echo "echo 'Type \"kubsh\" to start using it.'" >> $@
	@echo "# Update man database" >> $@
	@echo "if command -v mandb > /dev/null; then" >> $@
	@echo "    mandb" >> $@
	@echo "fi" >> $@
	chmod 755 $@

# Create pre-removal script (опционально)
$(DEB_DIR)/DEBIAN/prerm:
	@echo "#!/bin/bash" > $@
	@echo "# Pre-removal script for kubsh" >> $@
	@echo "echo 'Removing kubsh...'" >> $@
	chmod 755 $@

# Create copyright file
$(DEB_DIR)/usr/share/doc/kubsh/copyright:
	@echo "Creating copyright file..."
	@echo "Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/" > $@
	@echo "Upstream-Name: kubsh" >> $@
	@echo "Source: https://github.com/yourusername/kubsh" >> $@
	@echo "" >> $@
	@echo "Files: *" >> $@
	@echo "Copyright: $(shell date +%Y) Your Name" >> $@
	@echo "License: MIT" >> $@
	@echo " Permission is hereby granted, free of charge, to any person obtaining a copy" >> $@
	@echo " of this software and associated documentation files (the \"Software\"), to deal" >> $@
	@echo " in the Software without restriction, including without limitation the rights" >> $@
	@echo " to use, copy, modify, merge, publish, distribute, sublicense, and/or sell" >> $@
	@echo " copies of the Software, and to permit persons to whom the Software is" >> $@
	@echo " furnished to do so, subject to the following conditions:" >> $@
	@echo "" >> $@
	@echo " The above copyright notice and this permission notice shall be included in all" >> $@
	@echo " copies or substantial portions of the Software." >> $@

# Create changelog
$(DEB_DIR)/usr/share/doc/kubsh/changelog.Debian:
	@echo "Creating changelog..."
	@echo "kubsh (1.0-1) unstable; urgency=medium" > $@
	@echo "" >> $@
	@echo "  * Initial release" >> $@
	@echo "  * Basic shell functionality" >> $@
	@echo "" >> $@
	@echo " -- Your Name <your.email@example.com>  $(shell date -R)" >> $@
	@gzip -f $@

# Build the deb package
deb: install $(DEB_DIR)/DEBIAN/control $(DEB_DIR)/usr/share/man/man1/kubsh.1 $(DEB_DIR)/usr/share/doc/kubsh/copyright $(DEB_DIR)/usr/share/doc/kubsh/changelog.Debian $(DEB_DIR)/DEBIAN/postinst $(DEB_DIR)/DEBIAN/prerm
	@echo "Building deb package..."
	dpkg-deb --build $(DEB_DIR) $(DEB_NAME)
	@echo "Package created: $(DEB_NAME)"
	@echo "File size: $(shell du -h $(DEB_NAME) | cut -f1)"

# Quick deb build (минимальная версия)
quick-deb: build
	@mkdir -p $(DEB_DIR)/DEBIAN
	@mkdir -p $(DEB_DIR)/usr/bin
	@echo "Package: kubsh" > $(DEB_DIR)/DEBIAN/control
	@echo "Version: 1.0" >> $(DEB_DIR)/DEBIAN/control
	@echo "Section: utils" >> $(DEB_DIR)/DEBIAN/control
	@echo "Priority: optional" >> $(DEB_DIR)/DEBIAN/control
	@echo "Architecture: amd64" >> $(DEB_DIR)/DEBIAN/control
	@echo "Maintainer: Your Name <email@example.com>" >> $(DEB_DIR)/DEBIAN/control
	@echo "Description: Kubernetes Shell" >> $(DEB_DIR)/DEBIAN/control
	cp $(BUILD_DIR)/$(TARGET) $(DEB_DIR)/usr/bin/
	dpkg-deb --build $(DEB_DIR) $(DEB_NAME)
	@echo "Quick package created: $(DEB_NAME)"

# Install dependencies for deb building
install-deps:
	sudo apt-get update
	sudo apt-get install -y build-essential dpkg-dev

# Test the compiled binary
test: build
	@echo "Testing kubsh binary..."
	@echo "Binary path: $(BUILD_DIR)/$(TARGET)"
	@echo "File info:"
	@file $(BUILD_DIR)/$(TARGET)
	@echo "Running with --help or -h (if supported):"
	@$(BUILD_DIR)/$(TARGET) --help 2>/dev/null || $(BUILD_DIR)/$(TARGET) -h 2>/dev/null || echo "No help option implemented"

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(DEB_DIR)
	rm -f *.deb

# Install the program locally
local-install: build
	sudo cp $(BUILD_DIR)/$(TARGET) /usr/local/bin/$(TARGET)
	sudo chmod 755 /usr/local/bin/$(TARGET)
	@echo "Installed to /usr/local/bin/$(TARGET)"

# Uninstall locally
local-uninstall:
	sudo rm -f /usr/local/bin/$(TARGET)
	@echo "Uninstalled from /usr/local/bin/$(TARGET)"

# Check deb package contents
check-deb: $(DEB_NAME)
	@echo "Package contents:"
	dpkg-deb -c $(DEB_NAME)

# Show package info
info-deb: $(DEB_NAME)
	@echo "Package info:"
	dpkg-deb -I $(DEB_NAME)

# Install and test the deb package
install-test: deb
	sudo dpkg -i $(DEB_NAME) || true
	@echo ""
	@echo "Testing installation:"
	@which kubsh || echo "kubsh not found in PATH"
	@kubsh --version 2>/dev/null || echo "Could not run kubsh"

.PHONY: all build install deb quick-deb clean local-install local-uninstall check-deb info-deb prepare-deb install-deps test install-test
