CC      = gcc
CFLAGS  = -g -Wall -Wextra
OBJS    = main.o lexer.o token.o parser.o arena.o vector.o node.o node_arena.o ir.o analyzer.o asm_x86_64.o ir_bb.o ir_bb_arena.o ir_inst.o map.o type.o type_arena.o cc.o
TARGET  = cc

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: clean
