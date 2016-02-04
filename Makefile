LIBJIT_PATH = /usr/local/lib/
LIBJIT_INCLUDE_PATH = /usr/local/include/jit/jit
LIBJIT_AR = $(LIBJIT_PATH)/libjit.a

OBJ_DIR = obj
SRC_DIR = src

CC = gcc
LD = gcc
CFLAGS = -c $(CCOPT) -I$(LIBJIT_INCLUDE_PATH) -I.
LDFLAGS = -lpthread -lm -ldl $(LIBJIT_AR)
CCOPT = -g -O0
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(addprefix $(OBJ_DIR)/, $(notdir $(SOURCES:.c=.o)))

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@$(CC) $(CFLAGS) -c -o $@ $<
	@echo -e 'CC $<'

vm: create_dir $(OBJ_FILES)
	@$(LD) -o $@ $(OBJ_FILES) $(LDFLAGS)

debug: setdebugvars all

setdebugvars:
	@$(eval CFLAGS += $(SANITIZER_FLAGS) -DDEBUG -g -O0)
	@$(eval LDFLAGS += $(SANITIZER_LDFLAGS))

# vm: create_dirs $(OBJ_FILES)
	# $(LD) $^ $(LDFLAGS) -o $@


create_dir:
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR)

all: vm