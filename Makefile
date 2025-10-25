# Makefile - build common objects and two programs: index_server and ui_client
CC ?= gcc
CFLAGS ?= -std=c11 -O2 -D_POSIX_C_SOURCE=200112L -g -Wall -Wextra -I./src -I./src/common -I./src/client -I./src/server
LDFLAGS ?=

SRCDIR := src
BUILD_DIR := build
OBJDIR := $(BUILD_DIR)/obj

# Main sources for programs (override on the make command line if needed)
# e.g. make SERVER_MAIN=src/server/some_other_server.c UI_MAIN=src/client/some_client.c
SERVER_MAIN ?= $(SRCDIR)/server/index_server.c
UI_MAIN     ?= $(SRCDIR)/client/ui_client.c

# discover all .c files under src (recursive)
ALL_SRCS := $(shell find $(SRCDIR) -type f -name '*.c' | sort)

# common sources = all sources minus the two mains
COMMON_SRCS := $(filter-out $(SERVER_MAIN) $(UI_MAIN), $(ALL_SRCS))

# map src/.../*.c -> build/obj/.../*.o
COMMON_OBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(COMMON_SRCS))

# final executables
SERVER_EXE := $(BUILD_DIR)/index_server
UI_EXE     := $(BUILD_DIR)/ui_client

.PHONY: all clean rebuild dirs show

all: dirs $(SERVER_EXE) $(UI_EXE)

dirs:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(OBJDIR)

# compile common sources -> objects
# note: we create the parent directory for each object as needed
$(OBJDIR)/%.o: $(SRCDIR)/%.c | dirs
	@echo "CC -> $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

# link server: SERVER_MAIN + common objects
# we pass the .c SERVER_MAIN directly to the compiler (it will be compiled+linked)
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
	@echo "ALL_SRCS ="
	@echo "$(ALL_SRCS)"
	@echo "COMMON_SRCS ="
	@echo "$(COMMON_SRCS)"
	@echo "COMMON_OBJS ="
	@echo "$(COMMON_OBJS)"
	@echo "SERVER_EXE = $(SERVER_EXE)"
	@echo "UI_EXE = $(UI_EXE)"
