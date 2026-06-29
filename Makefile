ifeq ($(strip $(HOSTTYPE)),)
HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif

NAME := libft_malloc_$(HOSTTYPE).so
MAKEFLAGS += -j

LIBFT_DIR := libft
LIBFT     := $(LIBFT_DIR)/libft.a
LIBFT_D   := $(LIBFT_DIR)/libft_d.a

CC         ?= cc
CPPFLAGS   := -I include -I $(LIBFT_DIR)/include
CFLAGS     := -std=c11 -D_DEFAULT_SOURCE                                       \
			  -Wall -Wextra -Werror -Wshadow -Wpedantic                        \
			  -Wconversion -Wsign-conversion -Wdouble-promotion                \
			  -Wformat=2 -Wformat-security                                     \
			  -Wnull-dereference -Wcast-align -Wswitch-enum -Wundef            \
			  -Wstrict-prototypes -Wmissing-prototypes                         \
			  -Wredundant-decls -Wwrite-strings                                \
			  -Wimplicit-fallthrough                                           \
			  -Wcast-qual                                                      \
			  -Wvla -Walloca -Wold-style-definition

DEPSFLAGS := -MMD -MP

R_CFLAGS  := -DNDEBUG -O3 -march=native -fomit-frame-pointer -fPIC -fstack-clash-protection
R_LDFLAGS := -shared -Wl,-z,relro,-z,now

SANITIZERS := -fsanitize=address,undefined,null,leak,integer-divide-by-zero,signed-integer-overflow
D_CFLAGS   := -g3 -fno-omit-frame-pointer -fstack-protector-strong $(SANITIZERS)
D_LDFLAGS  := $(SANITIZERS) -rdynamic

SRC_DIR   := src
SRC_FILES := main.c
SRCS      := $(addprefix $(SRC_DIR)/, $(SRC_FILES))

R_OBJ_DIR := obj
R_OBJECTS := $(SRCS:$(SRC_DIR)/%.c=$(R_OBJ_DIR)/%.o)
R_DEPS    := $(R_OBJECTS:.o=.d)

D_OBJ_DIR := obj_debug
D_OBJECTS := $(SRCS:$(SRC_DIR)/%.c=$(D_OBJ_DIR)/%.o)
D_DEPS    := $(D_OBJECTS:.o=.d)

BLUE := \033[36m
MARGENTA := \033[35m
NC := \033[0m

.PHONY: all
all: $(NAME)  ## Build release version (default)

.PHONY: clean
clean:  ## Clean object files
	@$(MAKE) -C $(LIBFT_DIR) clean
	@rm -rf $(R_OBJ_DIR) $(D_OBJ_DIR) $(B_OBJ_DIR) $(BD_OBJ_DIR)

.PHONY: fclean
fclean:  ## Clean object, bin
	@$(MAKE) clean
	@$(MAKE) -C $(LIBFT_DIR) fclean
	@rm -f $(NAME) $(NAME_D) $(NAME_B) $(NAME_B_D)

.PHONY: re
re:  ## Clean all and recompile
	@$(MAKE) fclean
	@$(MAKE) all

.PHONY: fmt
fmt:  ## Format code via clang-format
	@echo "Format code"
	@find . -type f -name "*.c" -print0 | xargs -0 clang-format -i
	@find . -type f -name "*.h" -print0 | xargs -0 clang-format -i

.PHONY: help
help:  ## Get help
	@echo -e 'Usage: make ${BLUE}<target>${NC}'
	@echo -e 'Available targets:'
	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z_-]+:.*?## / {printf "  ${BLUE}%-15s${NC} %s\n", $$1, $$2}' $(MAKEFILE_LIST)

# Release build
$(NAME): $(LIBFT) $(R_OBJECTS)
	$(CC) $(R_OBJECTS) $(R_LDFLAGS) -s $(LIBFT) -o $@
	@echo "Build complete: $(NAME) (release)"

# Release pattern rule
$(R_OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(R_OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(R_CFLAGS) $(DEPSFLAGS) -c $< -o $@

$(LIBFT):
	@$(MAKE) -C $(LIBFT_DIR)

$(R_OBJ_DIR):
	@mkdir -p $@

-include $(R_DEPS)
