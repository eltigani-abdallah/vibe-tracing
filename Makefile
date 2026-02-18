NAME := raytracer

CC := cc
CFLAGS := -std=c11 -O2 -Wall -Wextra -Wpedantic
CPPFLAGS := -Iinclude

SRC_DIR := src
BUILD_DIR := build

SRCS := \
	$(SRC_DIR)/main.c \
	$(SRC_DIR)/app.c

OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

SDL_CFLAGS := $(shell sdl2-config --cflags 2>nul)
SDL_LIBS := $(shell sdl2-config --libs 2>nul)
ifeq ($(strip $(SDL_CFLAGS)),)
SDL_CFLAGS := $(shell pkg-config --cflags sdl2 2>nul)
SDL_LIBS := $(shell pkg-config --libs sdl2 2>nul)
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
	@mkdir $(BUILD_DIR) 2>nul || (exit 0)

clean:
	@rmdir /s /q $(BUILD_DIR) 2>nul || (exit 0)

fclean: clean
	@del /q $(NAME).exe 2>nul || (exit 0)
	@del /q $(NAME) 2>nul || (exit 0)

re: fclean all

run: all
	./$(NAME)
