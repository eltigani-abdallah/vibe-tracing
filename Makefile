NAME := raytracer

CC := cc
CFLAGS := -std=c11 -O2 -Wall -Wextra -Wpedantic
CPPFLAGS := -Iinclude

SRC_DIR := src
BUILD_DIR := build

SRCS := \
	$(SRC_DIR)/main.c \
	$(SRC_DIR)/app.c \
	$(SRC_DIR)/camera.c \
	$(SRC_DIR)/render.c

OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

SDL_CFLAGS := $(shell sdl2-config --cflags 2>/dev/null)
SDL_LIBS := $(shell sdl2-config --libs 2>/dev/null)
ifeq ($(strip $(SDL_CFLAGS)),)
SDL_CFLAGS := $(shell pkg-config --cflags sdl2 2>/dev/null)
SDL_LIBS := $(shell pkg-config --libs sdl2 2>/dev/null)
endif

CPPFLAGS += $(SDL_CFLAGS)
LDLIBS += $(SDL_LIBS)

.PHONY: all clean fclean re run

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDLIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

clean:
	@rm -rf $(BUILD_DIR)

fclean: clean
	@rm -f $(NAME) $(NAME).exe

re: fclean all

run: all
	./$(NAME)
