# Makefile simple pour projet SDL2
# Fenêtre de base avec buffer de pixels

CC = gcc
CFLAGS = -std=c99 -D_GNU_SOURCE -Wall -Wextra -O2 `sdl2-config --cflags`
LDFLAGS = `sdl2-config --libs`

# Fichiers source
SRCDIR = src
SOURCES = $(SRCDIR)/main.c $(SRCDIR)/camera.c
TARGET = sdl_app

# Binaires de test (sans SDL)
VEC3_TEST_SRC    = $(SRCDIR)/vec3.c
VEC3_TEST        = vec3_test

CAMERA_TEST_SRC  = $(SRCDIR)/camera.c
CAMERA_TEST      = camera_test

ALL_TESTS = $(VEC3_TEST) $(CAMERA_TEST)

.PHONY: all clean run test test-vec3 test-camera

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -lm -o $(TARGET) $(LDFLAGS)

# Test vec3
$(VEC3_TEST): $(VEC3_TEST_SRC) $(SRCDIR)/vec3.h
	$(CC) -std=c99 -Wall -Wextra -O2 -lm -DRUN_TESTS $(VEC3_TEST_SRC) -o $(VEC3_TEST)

test-vec3: $(VEC3_TEST)
	./$(VEC3_TEST)

# Test camera + ray
$(CAMERA_TEST): $(CAMERA_TEST_SRC) $(SRCDIR)/camera.h $(SRCDIR)/ray.h $(SRCDIR)/vec3.h
	$(CC) -std=c99 -Wall -Wextra -O2 -lm -DRUN_TESTS $(CAMERA_TEST_SRC) -o $(CAMERA_TEST)

test-camera: $(CAMERA_TEST)
	./$(CAMERA_TEST)

# Lance tous les tests
test: test-vec3 test-camera

clean:
	rm -f $(TARGET) $(ALL_TESTS)

run: $(TARGET)
	./$(TARGET)

install-sdl:
	@echo "📦 Installation de SDL2..."
	@if command -v brew >/dev/null 2>&1; then \
		brew install sdl2; \
	elif command -v apt-get >/dev/null 2>&1; then \
		sudo apt-get update && sudo apt-get install libsdl2-dev; \
	else \
		echo "⚠️  Veuillez installer SDL2 manuellement"; \
	fi

help:
	@echo "Projet SDL2 - Fenêtre de base"
	@echo "=============================="
	@echo "Targets disponibles:"
	@echo "  all        - Compiler le programme"
	@echo "  run        - Compiler et lancer"
	@echo "  clean      - Nettoyer les fichiers"
	@echo "  install-sdl - Installer SDL2"
	@echo ""
	@echo "Livrable: Fenêtre noire 800x600 qui se ferme proprement"