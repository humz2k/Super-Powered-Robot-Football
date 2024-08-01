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

ODE_DIR ?= ode
ODE_NIX_LIB ?= $(ODE_DIR)/ode/src/.libs/libode.a
ODE_LIB ?=
ODE_INCLUDE ?= $(ODE_DIR)/include

MINI_DIR ?= mini
MINI_INCLUDE ?= $(MINI_DIR)/include

DRIVERS_DIR ?= drivers

ENET_FLAGS ?=

RAYLIB_FLAGS ?= UNSUPPORTED_PLATFORM
ifeq ($(PLATFORM_OS), WINDOWS)
	RAYLIB_FLAGS = $(RAYLIB_WINDOWS_FLAGS)
	ENET_FLAGS = -lws2_32 -lwinmm
endif
ifeq ($(PLATFORM_OS), OSX)
	RAYLIB_FLAGS = $(RAYLIB_OSX_FLAGS)
	ODE_LIB = $(ODE_NIX_LIB)
endif


DEBUG_FLAGS ?= -g -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -fno-inline

FLAGS ?= -O3 -Wall -Wpedantic -Wno-newline-eof -Wno-c99-extensions -Wno-format-security -Wno-unused-function -Wno-unused-private-field -fPIC $(DEBUG_FLAGS)

SOURCE_DIR ?= src
BUILD_DIR ?= build

SOURCES := $(shell find $(SOURCE_DIR) -name '*.cpp') $(shell find $(SOURCE_DIR) -name '*.c')
OBJECTS := $(SOURCES:%.cpp=$(BUILD_DIR)/%.o)
OBJECTS := $(OBJECTS:%.c=$(BUILD_DIR)/%.o)

HEADERS := $(shell find $(SOURCE_DIR) -name '*.hpp') $(shell find $(DRIVERS_DIR) -name '*.hpp')

.PHONY: main
main: $(BUILD_DIR)/game $(BUILD_DIR)/server $(BUILD_DIR)/test

.PHONY: game
game: $(BUILD_DIR)/game

.PHONY: server
server: $(BUILD_DIR)/server

$(BUILD_DIR)/server: $(BUILD_DIR)/$(DRIVERS_DIR)/server.o $(OBJECTS) $(RAYLIB_DIR)/libraylib.a $(ODE_LIB)
	$(CXX) $^ -o $@ $(RAYLIB_FLAGS) $(FLAGS) $(ENET_FLAGS)

$(BUILD_DIR)/game: $(BUILD_DIR)/$(DRIVERS_DIR)/game.o $(OBJECTS) $(RAYLIB_DIR)/libraylib.a
	$(CXX) $^ -o $@ $(RAYLIB_FLAGS) $(FLAGS) $(ENET_FLAGS)

$(BUILD_DIR)/test: $(BUILD_DIR)/$(DRIVERS_DIR)/test.o $(OBJECTS) $(RAYLIB_DIR)/libraylib.a
	$(CXX) $^ -o $@ $(RAYLIB_FLAGS) $(FLAGS) $(ENET_FLAGS)

$(ODE_NIX_LIB):
	cd $(ODE_DIR) && $(MAKE)

.secondary: $(OBJECTS)

$(BUILD_DIR)/%.o: %.cpp $(HEADERS)
	mkdir -p $(@D)
	$(CXX) -c $< -o $@ -I$(RAYLIB_CPP_DIR) -I$(RAYLIB_DIR) -I$(SOURCE_DIR) -I$(ODE_INCLUDE) -I$(MINI_INCLUDE) -std=c++17 $(FLAGS)

$(BUILD_DIR)/%.o: %.c $(HEADERS)
	mkdir -p $(@D)
	$(CC) -c $< -o $@ -I$(RAYLIB_CPP_DIR) -I$(RAYLIB_DIR) -I$(SOURCE_DIR) -I$(ODE_INCLUDE) -I$(MINI_INCLUDE) $(FLAGS)


$(RAYLIB_DIR)/libraylib.a:
	cd $(RAYLIB_DIR) && $(MAKE) MACOSX_DEPLOYMENT_TARGET=10.9

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

.PHONY: fresh
fresh: clean
	cd $(RAYLIB_DIR) && $(MAKE) clean
	cd $(ODE_DIR) && $(MAKE) clean