# remember to configure ODE and enet

PLATFORM_OS ?= UNKNOWN

ifeq ($(OS),Windows_NT)
	PLATFORM_OS = WINDOWS
else
	UNAMEOS = $(shell uname)
	ifeq ($(UNAMEOS), Darwin)
		PLATFORM_OS = OSX
	endif
endif

RAYLIB_DIR ?= raylib/src
RAYLIB_CPP_DIR ?= raylib-cpp/include
RAYLIB_OSX_FLAGS ?= -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL
RAYLIB_WINDOWS_FLAGS ?= -lopengl32 -lgdi32 -lwinmm

ENET_DIR ?= enet
ENET_MAC_LIB ?= $(ENET_DIR)/.libs/libenet.a
ENET_LIB ?= $(ENET_MAC_LIB)
ENET_INCLUDE ?= $(ENET_DIR)/include

ODE_DIR ?= ODE
ODE_NIX_LIB ?= $(ODE_DIR)/ode/src/.libs/libode.a
ODE_LIB ?=
ODE_INCLUDE ?= $(ODE_DIR)/include

DRIVERS_DIR ?= drivers

RAYLIB_FLAGS ?= UNSUPPORTED_PLATFORM
ifeq ($(PLATFORM_OS), WINDOWS)
	RAYLIB_FLAGS = $(RAYLIB_WINDOWS_FLAGS)
	ENET_LIB = $(ENET_DIR)/enet64.lib
endif
ifeq ($(PLATFORM_OS), OSX)
	RAYLIB_FLAGS = $(RAYLIB_OSX_FLAGS)
	ENET_LIB = $(ENET_MAC_LIB)
	ODE_LIB = $(ODE_NIX_LIB)
endif


DEBUG_FLAGS ?= -g -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -fno-inline

FLAGS ?= -O3 -Wall -Wpedantic -Wno-newline-eof -Wno-c99-extensions -Wno-format-security -Wno-unused-function -Wno-unused-private-field -Werror -fPIC # $(DEBUG_FLAGS)

SOURCE_DIR ?= src
BUILD_DIR ?= build

SOURCES := $(shell find $(SOURCE_DIR) -name '*.cpp')
OBJECTS := $(SOURCES:%.cpp=$(BUILD_DIR)/%.o)

HEADERS := $(shell find $(SOURCE_DIR) -name '*.hpp')

main: $(BUILD_DIR)/game $(BUILD_DIR)/server

$(BUILD_DIR)/%: $(BUILD_DIR)/$(DRIVERS_DIR)/%.o $(OBJECTS) $(RAYLIB_DIR)/libraylib.a $(ENET_LIB) $(ODE_LIB)
	$(CXX) $^ -o $@ $(RAYLIB_FLAGS) $(FLAGS)

$(ENET_MAC_LIB):
	cd $(ENET_DIR) && $(MAKE)

$(ODE_NIX_LIB):
	cd $(ODE_DIR) && $(MAKE)

.secondary: $(OBJECTS)

$(BUILD_DIR)/%.o: %.cpp $(HEADERS)
	mkdir -p $(@D)
	$(CXX) -c $< -o $@ -I$(RAYLIB_CPP_DIR) -I$(RAYLIB_DIR) -I$(SOURCE_DIR) -I$(ENET_INCLUDE) -I$(ODE_INCLUDE) -std=c++17 $(FLAGS)

$(RAYLIB_DIR)/libraylib.a:
	cd $(RAYLIB_DIR) && $(MAKE) MACOSX_DEPLOYMENT_TARGET=10.9

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

.PHONY: fresh
fresh: clean
	cd $(RAYLIB_DIR) && $(MAKE) clean
	cd $(ENET_DIR) && $(MAKE) clean
	cd $(ODE_DIR) && $(MAKE) clean