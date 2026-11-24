.PHONY: all build debug relwithdebinfo clean format help
all: build

build:
	mkdir -p build
	@echo "Building project in Release mode..."
	cd build && \
	cmake -DCMAKE_BUILD_TYPE=Release .. && \
	make

debug:
	mkdir -p build
	@echo "Building project in Debug mode..."
	cd build && \
	cmake -DCMAKE_BUILD_TYPE=Debug .. && \
	make

relwithdebinfo:
	mkdir -p build
	@echo "Building project in RelWithDebInfo mode..."
	cd build && \
	cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo .. && \
	make

clean:
	rm -rf build

format:
	@echo "Formatting code..."
	clang-format Core/src/*.h Core/src/*.cpp Solver/src/*.h Solver/src/*.cpp -i --style=file

help:
	@echo "Available targets:"
	@echo "  all: Build and run tests"
	@echo "  build: Build the project in release mode"
	@echo "  test: Build and run tests"
	@echo "  debug: Build the project in debug mode"

test:
	mkdir -p build
	@echo "Building and running tests..."
	cd build && \
	cmake -DCMAKE_BUILD_TYPE=Debug .. && \
	make && \
	cd Solver/tests && \
	ctest --output-on-failure
