RAYLIB_DIR ?= raylib/src
RAYLIB_CPP_DIR ?= raylib-cpp/include
RAYLIB_OSX_FLAGS ?= -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL

RAYLIB_FLAGS := $(RAYLIB_OSX_FLAGS)

FLAGS ?= -Wall -Wpedantic -Wno-newline-eof -fPIC

SOURCE_DIR ?= src
BUILD_DIR ?= build

SOURCES := $(shell find $(SOURCE_DIR) -name '*.cpp')
OBJECTS := $(SOURCES:%.cpp=$(BUILD_DIR)/%.o)

HEADERS := $(shell find $(SOURCE_DIR) -name '*.hpp')

main: $(OBJECTS) $(RAYLIB_DIR)/libraylib.a
	$(CXX) $^ -o $(BUILD_DIR)/game $(RAYLIB_FLAGS)

.secondary: $(OBJECTS)

$(BUILD_DIR)/%.o: %.cpp $(HEADERS)
	mkdir -p $(@D)
	$(CXX) -c $< -o $@ -I$(RAYLIB_CPP_DIR) -I$(RAYLIB_DIR) -I$(SOURCE_DIR) -std=c++17

$(RAYLIB_DIR)/libraylib.a:
	cd $(RAYLIB_DIR) && $(MAKE) MACOSX_DEPLOYMENT_TARGET=10.9 CUSTOM_CFLAGS=-fno-inline

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	cd $(RAYLIB_DIR) && $(MAKE) clean