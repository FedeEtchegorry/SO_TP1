include ../makefile.globals

UTILS_SRCS := $(wildcard *.c)
UTILS_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(UTILS_SRCS))

all: $(UTILS_OBJS)

$(OBJ_DIR):
	mkdir $(@)

$(OBJ_DIR)/%o: %c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

.PHONY: all clean
