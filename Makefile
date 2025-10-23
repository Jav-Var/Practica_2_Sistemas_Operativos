# Makefile - build common objects and two programs: index_server and ui_client
CC ?= gcc
CFLAGS ?= -std=c11 -O2 -g -Wall -Wextra -I./src
LDFLAGS ?=

SRCDIR := src
BUILD_DIR := build
OBJDIR := $(BUILD_DIR)/obj

# Main sources for programs (override on the make command line if needed)
# e.g. make SERVER_MAIN=examples/index_server.c UI_MAIN=examples/ui_client.c
SERVER_MAIN ?= $(SRCDIR)/index_server.c
UI_MAIN     ?= $(SRCDIR)/ui_client.c

# all .c in src
ALL_SRCS := $(wildcard $(SRCDIR)/*.c)

# treat SERVER_MAIN and UI_MAIN as mains; build common sources = ALL_SRCS minus mains (if present)
COMMON_SRCS := $(filter-out $(SERVER_MAIN) $(UI_MAIN), $(ALL_SRCS))

# objects for common sources
COMMON_OBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(COMMON_SRCS))

# ensure build targets
SERVER_EXE := $(BUILD_DIR)/index_server
UI_EXE     := $(BUILD_DIR)/ui_client

.PHONY: all clean rebuild dirs show

all: dirs $(SERVER_EXE) $(UI_EXE)

dirs:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(OBJDIR)

# compile common sources -> objects
$(OBJDIR)/%.o: $(SRCDIR)/%.c | dirs
	@echo "CC -> $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# link server: SERVER_MAIN + common objects
$(SERVER_EXE): $(COMMON_OBJS) $(SERVER_MAIN) | dirs
	@echo "LINK -> $(SERVER_EXE)"
	@$(CC) $(CFLAGS) $(COMMON_OBJS) $(SERVER_MAIN) $(LDFLAGS) -o $(SERVER_EXE)

# link ui client: UI_MAIN + common objects
$(UI_EXE): $(COMMON_OBJS) $(UI_MAIN) | dirs
	@echo "LINK -> $(UI_EXE)"
	@$(CC) $(CFLAGS) $(COMMON_OBJS) $(UI_MAIN) $(LDFLAGS) -o $(UI_EXE)

clean:
	@echo "Cleaning $(BUILD_DIR)"
	@rm -rf $(BUILD_DIR)

rebuild: clean all

show:
	@echo "CC = $(CC)"
	@echo "CFLAGS = $(CFLAGS)"
	@echo "SERVER_MAIN = $(SERVER_MAIN)"
	@echo "UI_MAIN = $(UI_MAIN)"
	@echo "COMMON_SRCS = $(COMMON_SRCS)"
	@echo "COMMON_OBJS = $(COMMON_OBJS)"
	@echo "SERVER_EXE = $(SERVER_EXE)"
	@echo "UI_EXE = $(UI_EXE)"
