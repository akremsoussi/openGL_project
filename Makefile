# ARAM - OpenGL 3D scene
#
# Dependencies (Debian / Ubuntu):
#   sudo apt install build-essential pkg-config libglfw3-dev libglew-dev libglm-dev
#
#   make        build ./aram
#   make run    build and run (from this folder, so resources/ is found)
#   make clean  remove the build files
#
# Folders:
#   src/            this project, one folder per module (see README.md)
#   IOGL-homework/  given code: camera, OBJ mesh, texture, shader classes
#   external/       header-only libraries: stb_image (images), cgltf (.glb files),
#                   miniaudio (sound: MP3 decoding and playback)
#   resources/      models, textures and shaders loaded at run time

TARGET   := aram
HW_DIR   := IOGL-homework/IOGL-homework
BUILD    := build

SRCS     := src/main.cpp \
            src/app/Application.cpp \
            src/audio/MusicPlayer.cpp \
            src/animation/Easing.cpp \
            src/animation/Timeline.cpp \
            src/camera/CinematicCamera.cpp \
            src/cinematic/CameraTour.cpp \
            src/graphics/HDRPipeline.cpp \
            src/graphics/ParticleRenderer.cpp \
            src/graphics/PointShadowMap.cpp \
            src/graphics/SkyLight.cpp \
            src/graphics/Renderer.cpp \
            src/graphics/ShadowMap.cpp \
            src/graphics/SkinnedModel.cpp \
            src/scene/LightManager.cpp \
            src/scene/SceneSetup.cpp \
            src/systems/ParticleSystem.cpp \
            src/systems/SnowEmitter.cpp \
            $(HW_DIR)/Camera.cpp \
            $(HW_DIR)/Mesh.cpp \
            $(HW_DIR)/ShaderProgram.cpp \
            $(HW_DIR)/Texture2D.cpp

# build/ mirrors the source folders: build/src/app/Application.o ...
OBJS     := $(SRCS:%.cpp=$(BUILD)/%.o)
DEPS     := $(OBJS:.o=.d)

CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -MMD -MP -Isrc -I$(HW_DIR) -Iexternal \
            $(shell pkg-config --cflags glfw3 glew 2>/dev/null)
# miniaudio: threads + dlopen (it loads the system sound library at run time)
LDLIBS   := $(shell pkg-config --libs glfw3 glew 2>/dev/null || echo -lglfw -lGLEW) -lGL -lpthread -lm -ldl

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDLIBS)

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# miniaudio (third-party) ignores a few return values on purpose
$(BUILD)/src/audio/MusicPlayer.o: CXXFLAGS += -Wno-unused-result

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD) $(TARGET)

-include $(DEPS)
