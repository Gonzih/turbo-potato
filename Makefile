CXX = clang++

DISTRO=$(shell sh -c "cat /etc/*-release | grep DISTRIB_ID | sed 's/.*=//'")
LDFLAGS += -std=c++2a
LDFLAGS += -lncurses

BIN_NAME = core

default: run

clean:
	rm -f ./$(BIN_NAME)

build: clean
	$(CXX) $(LDFLAGS) -o $(BIN_NAME) src/*.cpp
	chmod +x ./$(BIN_NAME)

build-nix:
	nix-shell shell.nix --run 'make build'

run: build
	./$(BIN_NAME)

run-nix: build-nix
	./$(BIN_NAME)

shell:
	nix-shell shell.nix
