CC:=g++
SRCEXT:=cpp
CFLAGS:=-std=c++17 -O3
LDFLAGS:=-lpng

BIN:=desktop-dmenu
SRC:=.
OBJ:=obj
MAKEFILE:=Makefile
CLANGDINFO:=compile_commands.json
DEPDIR:=$(OBJ)/deps

SRCS:=$(wildcard $(SRC)/*.$(SRCEXT))
DEPFLAGS=-MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

make:: $(CLANGDINFO) $(BIN)

run:: $(CLANGDINFO) $(BIN)
	./$(BIN)

clean::
	rm -r $(OBJ) || true
	rm -r $(DEPDIR) || true
	rm $(BIN) || true
	rm $(CLANGDINFO) || true

install:: $(BIN)
	cp $(BIN) /usr/local/bin/$(BIN)

# Rules for compilation
OBJS:=$(SRCS:$(SRC)/%.$(SRCEXT)=$(OBJ)/%.o)
$(BIN): $(OBJS) $(OBJ)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ)/%.o: $(SRC)/%.$(SRCEXT) $(OBJ) $(DEPDIR)/%.d | $(DEPDIR)
	$(CC) $(DEPFLAGS) $(CFLAGS) -c -o $@ $<

# Directories
$(OBJ):
	mkdir -p $@

$(DEPDIR):
	mkdir -p $@

# Dependencies (include files)
DEPFILES:=$(SRCS:$(SRC)/%.$(SRCEXT)=$(DEPDIR)/%.d)
$(DEPFILES):
include $(wildcard $(DEPFILES))

# Create compile commands using bear
makebin:: $(BIN)

$(CLANGDINFO): $(MAKEFILE)
	make clean
	bear -- make makebin
