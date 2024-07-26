RAYLIB_DIR ?= raylib/src
RAYLIB_CPP_DIR ?= raylib-cpp/include
RAYLIB_OSX_FLAGS ?= -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL

RAYLIB_FLAGS := $(RAYLIB_OSX_FLAGS)

DEBUG_FLAGS ?= -g -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -fno-inline

FLAGS ?= -Wall -Wpedantic -Wno-newline-eof -Wno-c99-extensions -Wno-format-security -Werror -fPIC $(DEBUG_FLAGS)

SOURCE_DIR ?= src
BUILD_DIR ?= build

SOURCES := $(shell find $(SOURCE_DIR) -name '*.cpp')
OBJECTS := $(SOURCES:%.cpp=$(BUILD_DIR)/%.o)

HEADERS := $(shell find $(SOURCE_DIR) -name '*.hpp')

main: $(OBJECTS) $(RAYLIB_DIR)/libraylib.a
	$(CXX) $^ -o $(BUILD_DIR)/game $(RAYLIB_FLAGS) $(FLAGS)

.secondary: $(OBJECTS)

$(BUILD_DIR)/%.o: %.cpp $(HEADERS)
	mkdir -p $(@D)
	$(CXX) -c $< -o $@ -I$(RAYLIB_CPP_DIR) -I$(RAYLIB_DIR) -I$(SOURCE_DIR) -std=c++17 $(FLAGS)

$(RAYLIB_DIR)/libraylib.a:
	cd $(RAYLIB_DIR) && $(MAKE) MACOSX_DEPLOYMENT_TARGET=10.9 CUSTOM_CFLAGS=-fno-inline

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	cd $(RAYLIB_DIR) && $(MAKE) clean