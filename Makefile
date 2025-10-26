# Makefile - build common objects and two programs: index_server and ui_client
# This version explicitly defines dependencies for client and server.
CC ?= gcc
CFLAGS ?= -std=c11 -O2 -D_POSIX_C_SOURCE=200112L -g -Wall -Wextra -I./src -I./src/common -I./src/client -I./src/server
LDFLAGS ?=
# LDFLAGS ?= -lnsl # Descomenta si tienes errores de 'undefined reference' a funciones de red

SRCDIR := src
BUILD_DIR := build
OBJDIR := $(BUILD_DIR)/obj

# --- 1. Define Source Files Explicitly ---

# COMMON: Código compartido por AMBOS
COMMON_SRCS := $(SRCDIR)/common/common.c \
               $(SRCDIR)/common/util.c

# SERVER: Código que solo usa el servidor
SERVER_CORE_SRCS := $(SRCDIR)/server/reader.c \
                    $(SRCDIR)/server/builder.c \
                    $(SRCDIR)/server/hash.c \
                    $(SRCDIR)/server/buckets.c \
                    $(SRCDIR)/server/arrays.c

# CLIENT: Código que solo usa el cliente
CLIENT_CORE_SRCS := $(SRCDIR)/client/ui.c

# MAINS: Los puntos de entrada de cada programa
SERVER_MAIN_SRC := $(SRCDIR)/server/index_server.c
CLIENT_MAIN_SRC := $(SRCDIR)/client/ui_client.c

# --- 2. Map Sources to Object Files ---
# (patsubst) convierte 'src/common/common.c' -> 'build/obj/common/common.o'

COMMON_OBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(COMMON_SRCS))
SERVER_CORE_OBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SERVER_CORE_SRCS))
CLIENT_CORE_OBJS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(CLIENT_CORE_SRCS))

SERVER_MAIN_OBJ := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SERVER_MAIN_SRC))
CLIENT_MAIN_OBJ := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(CLIENT_MAIN_SRC))

# --- 3. Define Full Object Lists for Linking ---

# El servidor necesita: common + server_core + server_main
SERVER_OBJS_LIST := $(COMMON_OBJS) $(SERVER_CORE_OBJS) $(SERVER_MAIN_OBJ)

# El cliente necesita: common + client_core + client_main
CLIENT_OBJS_LIST := $(COMMON_OBJS) $(CLIENT_CORE_OBJS) $(CLIENT_MAIN_OBJ)

# --- 4. Define Executable Paths ---

SERVER_EXE := $(BUILD_DIR)/index_server
UI_EXE     := $(BUILD_DIR)/ui_client

# --- 5. Build Rules ---

.PHONY: all clean rebuild dirs show

all: dirs $(SERVER_EXE) $(UI_EXE)

dirs:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(OBJDIR)/common
	@mkdir -p $(OBJDIR)/server
	@mkdir -p $(OBJDIR)/client

# Regla GENÉRICA para compilar CUALQUIER .c a su .o correspondiente
# Crea el subdirectorio del objeto (ej. build/obj/server) si no existe
$(OBJDIR)/%.o: $(SRCDIR)/%.c | dirs
	@echo "CC -> $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

# Regla de ENLACE (link) para el SERVIDOR
# Depende de su lista específica de objetos
$(SERVER_EXE): $(SERVER_OBJS_LIST)
	@echo "LINK -> $@"
	@$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

# Regla de ENLACE (link) para el CLIENTE
# Depende de su lista específica de objetos
$(UI_EXE): $(CLIENT_OBJS_LIST)
	@echo "LINK -> $@"
	@$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

clean:
	@echo "Cleaning $(BUILD_DIR)"
	@rm -rf $(BUILD_DIR)

rebuild: clean all

show:
	@echo "--- Build Configuration ---"
	@echo "SERVER EXE: $(SERVER_EXE)"
	@echo "SERVER OBJS: $(SERVER_OBJS_LIST)"
	@echo
	@echo "CLIENT EXE: $(UI_EXE)"
	@echo "CLIENT OBJS: $(CLIENT_OBJS_LIST)"