CC=g++
CFLAGS=-std=c++17 -ggdb
LDFLAGS=

MAKEFILE=Makefile
CLANGDINFO=compile_commands.json

SRC=.
SRCS=$(wildcard $(SRC)/*.cpp)

OBJ=obj
OBJS=$(patsubst $(SRC)/%.cpp, $(OBJ)/%.o, $(SRCS))

BIN=desktop-dmenu

make: $(CLANGDINFO) $(BIN)

run: $(BIN)
	./$(BIN)

clean:
	rm -r $(OBJ) || true
	rm $(BIN) || true
	rm $(CLANGDINFO) || true

install: make
	cp $(BIN) /usr/local/bin/$(BIN)

# Rules for compilation

$(BIN): $(OBJS) $(OBJ)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ)/%.o: $(SRC)/%.cpp $(OBJ)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ):
	mkdir -p $@


# Create compile commands
makebin: $(BIN)

$(CLANGDINFO): $(MAKEFILE)
	make clean
	bear -- make makebin
