CXX      ?= c++
CXXFLAGS  = -std=c++17 -O2 -Wall -Wextra -Wpedantic -Wno-missing-field-initializers
SRCS      = src/main.cpp src/parser.cpp src/renderer.cpp
TARGET    = mdprev

# ── platform detection ────────────────────────────────────────────────────────
UNAME_S := $(shell uname -s 2>/dev/null || echo Windows)

ifeq ($(UNAME_S),Linux)
    INSTALL_DIR ?= /usr/local/bin
    INSTALL_CMD  = install -m 755
endif
ifeq ($(UNAME_S),Darwin)
    INSTALL_DIR ?= /usr/local/bin
    INSTALL_CMD  = install -m 755
endif
ifeq ($(UNAME_S),Windows)
    TARGET      := mdprev.exe
    INSTALL_DIR ?= $(USERPROFILE)/bin
    INSTALL_CMD  = cp
    CXXFLAGS    += -DWIN32_LEAN_AND_MEAN
endif

# ── default goal: build ───────────────────────────────────────────────────────
.PHONY: all build install uninstall clean

all: build

build: $(TARGET)

$(TARGET): $(SRCS) src/ansi.hpp src/parser.hpp src/renderer.hpp
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS)

# ── install / uninstall ───────────────────────────────────────────────────────
install: build
	@echo "Installing $(TARGET) to $(INSTALL_DIR)"
ifeq ($(UNAME_S),Windows)
	@if not exist "$(INSTALL_DIR)" mkdir "$(INSTALL_DIR)"
	$(INSTALL_CMD) $(TARGET) "$(INSTALL_DIR)/$(TARGET)"
else
	@mkdir -p $(INSTALL_DIR)
	$(INSTALL_CMD) $(TARGET) $(INSTALL_DIR)/$(TARGET)
endif
	@echo "Done. Make sure $(INSTALL_DIR) is in your PATH."

uninstall:
	@echo "Removing $(INSTALL_DIR)/$(TARGET)"
	@rm -f $(INSTALL_DIR)/$(TARGET)

# ── housekeeping ──────────────────────────────────────────────────────────────
clean:
	rm -f $(TARGET) mdprev.exe
