# ARAM - OpenGL

## Build and run (Linux / WSL)

```bash
sudo apt install build-essential pkg-config libglfw3-dev libglew-dev libglm-dev
make run
```

Run from this folder: the program loads everything from `resources/`.

Sound: the program plays through the system sound server (PulseAudio / PipeWire).
On WSL install its client library once, otherwise it runs without sound:

```bash
sudo apt install libpulse0
```

## Project structure

```
Makefile
src/                          code of this project, one folder per module
  main.cpp                    creates the Application and runs it
  app/Application             window, input, update and render loop (no globals)
  audio/MusicPlayer           plays the soundtrack in a loop, mute (miniaudio)
  scene/
    Transform                 position / rotation / scale -> model matrix
    Material                  shader + "receives lighting" + extra uniforms
    SceneObject, Scene        the objects to draw, found by name
    LightManager              moon, lamps (point lights) and flashlight -> shader
    SceneResources            shaders and models owned by the scene
    SceneSetup                builds the ARAM scene: paths, positions, lights
  graphics/
    Renderer                  draws the objects and the shadow maps
    ShadowMap                 shadows of the moon (orthographic) and of the flashlight (perspective)
    PointShadowMap            shadows of a lamp: depth cube map (6 directions)
    SkyLight                  the moon / sun disk in the sky, where the main light comes from
    HDRPipeline               HDR image + bloom (bright pass, blur, tone mapping)
    ParticleRenderer          all particles in one instanced draw call
    SkinnedModel              animated characters from .glb files (glTF): one or several
                              skinned meshes, metallic-roughness or specular-glossiness materials,
                              bone matrices sent in a uniform buffer (up to 256 bones)
  systems/
    ParticleSystem            moves and removes particles
    SnowEmitter               creates snowflakes around the camera
  animation/
    Easing, AnimationTrack    keyframes and interpolation curves
    Timeline, CameraTrack     playback clock driving the tracks
  camera/CinematicCamera      camera moved by keyframes (derived from the homework Camera)
  cinematic/CameraTour        the keyframes of the camera tour (46 s, ends on Trump)
IOGL-homework/IOGL-homework/  given code (compiled by the Makefile)
  Camera                      FPS camera (used as is) and base of CinematicCamera
  Mesh                        OBJ loader, extended to read .mtl materials
  Texture2D                   textures, extended to load images from memory
  ShaderProgram               shaders and uniforms
external/                     header-only libraries
  stb_image/                  image files (used by Texture2D)
  cgltf/                      .glb / .gltf files (used by SkinnedModel)
  miniaudio/                  MP3 decoding and sound playback (used by MusicPlayer)
resources/                    everything loaded at run time
  models/aram/                the map: aram.obj + aram.mtl + textures/
  models/characters/          animated .glb characters (mesh, skeleton, animation, textures):
                              zed, k_da_* (dancers), trump_silly_dancing
  models/dead/                dead champions: one folder each with .obj + .mtl + textures/
                              (loaded with the homework OBJ loader)
  models/light/               light bulb model (from the homework)
  sounds/                     KDA_Recall_SFX.mp3 (soundtrack)
  shaders/                    GLSL shaders
```

## One frame, step by step

At start, the shadows of what never moves are rendered once:
- the map seen from the moon (`shadow_depth.vert/.frag`, orthographic)
- the map and the dead bodies seen from each of the 3 lamps: 6 faces of a cube
  map storing the distance to the lamp (`point_shadow_depth.vert/.frag`)

1. **Update** (`Application::update`): camera (mouse/keys, or the tour timeline),
   animations of the objects marked `animated` (Zed, the dancers, Trump), flashlight, snow.
2. **Shadows that change** (every frame):
   - characters seen from the moon (`skinned_shadow_depth.vert`)
   - moving characters near each lamp, into the lamp's second cube map
     (`skinned_point_shadow_depth.vert`)
   - everything seen from the flashlight when it is on (perspective shadow map)
3. **Scene into the HDR image** (`Renderer::render`), each object with its material:
   - map and dead bodies: `lighting_dir_point_spot.vert` (homework) + `scene.frag`
   - animated characters: `skinned.vert` + `scene.frag`
   - lamps: `bulb.vert/.frag` (homework), color x4 so they glow
   - moon / sun disk: `sky_light.vert/.frag`, in the direction of the light
   - snow: `particle.vert/.frag`, one instanced draw, alpha blending (on by default, P)
4. **Bloom** (`HDRPipeline::finish`): bright parts (`bright.frag`), blurred
   8 times at half size (`blur.frag`), added back and tone mapped (`composite.frag`).

`scene.frag` is the homework Blinn-Phong shader (moon + up to 8 point lights +
flashlight) with material colors, transparent leaves and the shadows of every
light: 2 maps for the moon, 1 for the flashlight, 2 cube maps per lamp
(static + moving), all with soft edges (several samples averaged).

## Controls

| Key | Action |
|---|---|
| Mouse / arrows | look around |
| W A S D | move (Z Q S D on AZERTY) |
| Space / Left Ctrl | up / down |
| Left Shift | run |
| Mouse wheel | zoom |
| C | camera tour (start / stop); at the end it stays on Trump until C |
| N | day / night (starts by day) |
| O | shadows on / off (moon, lamps, flashlight) |
| G | bloom on / off |
| P | snow on / off |
| B | pause / play the animations |
| T | freeze / unfreeze time (animations, snow, tour, music); the camera still moves |
| M | mute / unmute the sound (it loops) |
| F | flashlight (with shadows) |
| F1 | wireframe |
| Esc | quit |
