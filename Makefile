CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Werror
LDFLAGS = -lfuse3

SRC = src/main.cpp src/vfs.cpp
TARGET = kubsh

PKG_DIR = kubsh-package
PKG_BIN = $(PKG_DIR)/usr/bin
DEB_NAME = kubsh.deb

.PHONY: build run deb clean

build: $(TARGET)

$(TARGET): $(SRC)
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)
	cp $(TARGET) build/

run: $(TARGET)
	./$(TARGET)

deb: build
	@echo "==> Preparing package directory"
	mkdir -p $(PKG_BIN)
	cp $(TARGET) $(PKG_BIN)/
	chmod 755 $(PKG_BIN)/$(TARGET)
	@echo "==> Building .deb package"
	dpkg-deb --build $(PKG_DIR) $(DEB_NAME)
	@echo "==> Done: $(DEB_NAME)"

clean:
	rm -rf build
	rm -f $(TARGET)
	rm -f $(DEB_NAME)
