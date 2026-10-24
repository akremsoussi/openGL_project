//-----------------------------------------------------------------------------
// The application: window, input, update and render loop
//-----------------------------------------------------------------------------
#include "app/Application.h"
#include "GLFW/glfw3.h"
#include "glm/gtc/matrix_transform.hpp"
#include "cinematic/CameraTour.h"
#include "scene/SceneSetup.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

namespace
{
	const char* APP_TITLE = "ARAM - OpenGL";

	const double ZOOM_SENSITIVITY = -3.0;
	const float MOVE_SPEED = 4.0f;			// units per second
	const float RUN_FACTOR = 3.0f;
	const float MOUSE_SENSITIVITY = 0.08f;	// degrees per pixel
	const float TURN_SPEED = 90.0f;			// arrow keys, degrees per second
	const float MAX_FRAME_TIME = 0.1f;		// longer frames (window moved...) are cut
}

//-----------------------------------------------------------------------------
// Constructor / destructor
//-----------------------------------------------------------------------------
Application::Application()
	: mWindow(NULL), mWindowWidth(1024), mWindowHeight(768), mWSL(false),
	  mWireframe(false), mNight(false), mShadowsOn(true), mBloomOn(true), mSnowOn(true), mAnimationOn(true), mFrozen(false),
	  mFPSCamera(SceneSetup::CAMERA_START), mTourMode(false), mLastMouseX(0.0), mLastMouseY(0.0), mFirstMouse(true),
	  mFPSStart(0.0), mFrameCount(0), mSnow(mParticles)
{
}

Application::~Application()
{
	shutdown();
}

//-----------------------------------------------------------------------------
// Window, OpenGL, then the scene, shadow maps, bloom, snow and camera tour
//-----------------------------------------------------------------------------
bool Application::init()
{
	if (!initOpenGL())
		return false;

	// Scene: resources, objects and lights
	mResources.reset(new SceneResources());
	if (!SceneSetup::build(mScene, mLights, *mResources))
		return false;

	// Starts by day (key N: night)
	SceneSetup::setTimeOfDay(mLights, mNight);

	int width, height;
	glfwGetFramebufferSize(mWindow, &width, &height);

	// The map and the light do not move: its shadow map is rendered only once
	mMapShadow.reset(new ShadowMap());
	mMapShadow->init(SceneSetup::MAP_SHADOW_SIZE);
	mMapShadow->setLight(SceneSetup::LIGHT_DIRECTION, SceneSetup::MAP_SHADOW_MIN, SceneSetup::MAP_SHADOW_MAX);
	mRenderer.renderShadows(mScene, ShadowGroup::Static, *mMapShadow, width, height);

	// The characters move: their shadow map is rendered every frame
	mCharacterShadow.reset(new ShadowMap());
	mCharacterShadow->init(SceneSetup::CHARACTER_SHADOW_SIZE);
	mCharacterShadow->setLight(SceneSetup::LIGHT_DIRECTION, SceneSetup::CHARACTER_SHADOW_MIN, SceneSetup::CHARACTER_SHADOW_MAX);

	// Flashlight: its shadow map follows the camera, rendered every frame while it is on
	mSpotShadow.reset(new ShadowMap());
	mSpotShadow->init(SceneSetup::SPOT_SHADOW_SIZE);

	// Lamps: two cube shadow maps each (same order as the lights in the shader).
	// The map and the still objects never move: drawn once, now.
	std::vector<PointShadowMap*> staticMaps, dynamicMaps;
	for (const PointLight& lamp : mLights.getPointLights())
	{
		if (!lamp.castsShadows || (int)mPointStatic.size() >= LightManager::MAX_POINT_SHADOWS)
			continue;

		PointShadowMap* still = new PointShadowMap();
		still->init(SceneSetup::POINT_SHADOW_SIZE, SceneSetup::POINT_SHADOW_FAR);
		still->setLightPosition(lamp.position);
		mRenderer.renderPointShadows(mScene, *still, false, 0.0f, width, height);

		PointShadowMap* moving = new PointShadowMap();
		moving->init(SceneSetup::POINT_SHADOW_DYNAMIC_SIZE, SceneSetup::POINT_SHADOW_FAR);
		moving->setLightPosition(lamp.position);

		mPointStatic.push_back(std::unique_ptr<PointShadowMap>(still));
		mPointDynamic.push_back(std::unique_ptr<PointShadowMap>(moving));
		staticMaps.push_back(still);
		dynamicMaps.push_back(moving);
	}
	mRenderer.setPointShadows(staticMaps, dynamicMaps, SceneSetup::POINT_SHADOW_FAR);

	// The main light made visible: moon / sun disk in the sky
	mSkyLight.reset(new SkyLight());
	if (!mSkyLight->init())
		mSkyLight.reset();

	// Bloom (without it the scene is drawn directly on the window)
	mHDR.reset(new HDRPipeline());
	if (!mHDR->init(width, height))
		mHDR.reset();

	// Snow
	if (mParticles.initGraphics() && mSnowOn)
		mSnow.prefill(SceneSetup::CAMERA_START);

	// Camera tour (key C)
	CameraTour::build(mTour, mTourCamera);

	// Compute the camera vectors before the first frame
	mFPSCamera.rotate(0.0f, 0.0f);

	// Soundtrack (the program runs without sound if there is no sound device)
	mMusic.play(SceneSetup::SOUNDTRACK);

	printControls();

	mFPSStart = glfwGetTime();
	return true;
}

//-----------------------------------------------------------------------------
// GLFW window + OpenGL 3.3 core context + GLEW
//-----------------------------------------------------------------------------
bool Application::initOpenGL()
{
	mWSL = (getenv("WSL_DISTRO_NAME") != NULL);

	// WSL: Mesa tries the Zink driver first, fails, and falls back to slow
	// software rendering.  Ask for the D3D12 driver to use the real GPU.
	if (mWSL)
		setenv("GALLIUM_DRIVER", "d3d12", 0);	// 0 = keep the user's value if already set

#if GLFW_VERSION_MAJOR > 3 || (GLFW_VERSION_MAJOR == 3 && GLFW_VERSION_MINOR >= 4)
	// GLFW 3.4 opens a Wayland (EGL) window when WAYLAND_DISPLAY is set (WSLg, GNOME...)
	// but GLEW is built for X11 (GLX): use X11 (XWayland) when it is available.
	if (glfwPlatformSupported(GLFW_PLATFORM_X11))
		glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
#endif

	if (!glfwInit())
	{
		std::cerr << "GLFW initialization failed" << std::endl;
		return false;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	mWindow = glfwCreateWindow(mWindowWidth, mWindowHeight, APP_TITLE, NULL, NULL);
	if (mWindow == NULL)
	{
		std::cerr << "Failed to create GLFW window" << std::endl;
		return false;
	}
	glfwMakeContextCurrent(mWindow);

	// With an EGL (Wayland) context, GLEW loads the OpenGL functions and then
	// fails on the missing GLX display: that error can be ignored.
	glewExperimental = GL_TRUE;
	GLenum glewStatus = glewInit();
	if (glewStatus != GLEW_OK && glewStatus != GLEW_ERROR_NO_GLX_DISPLAY)
	{
		std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(glewStatus) << std::endl;
		return false;
	}
	std::cout << "OpenGL " << glGetString(GL_VERSION) << " - " << glGetString(GL_RENDERER) << std::endl;

	// Callbacks: GLFW gives back the window, the object is found with the user pointer
	glfwSetWindowUserPointer(mWindow, this);
	glfwSetKeyCallback(mWindow, keyCallback);
	glfwSetFramebufferSizeCallback(mWindow, framebufferSizeCallback);
	glfwSetScrollCallback(mWindow, scrollCallback);

	if (mWSL)
	{
		// WSLg cannot lock or move the mouse pointer: only hide it over the window
		glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	}
	else
	{
		// Hides and grabs cursor, unlimited movement
		glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
#if GLFW_VERSION_MAJOR > 3 || (GLFW_VERSION_MAJOR == 3 && GLFW_VERSION_MINOR >= 3)
		if (glfwRawMouseMotionSupported())
			glfwSetInputMode(mWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
#endif
	}

	int width, height;
	glfwGetFramebufferSize(mWindow, &width, &height);	// For retina display
	glViewport(0, 0, width, height);
	glEnable(GL_DEPTH_TEST);

	// Show something while the (big) scene is loading
	glm::vec4 sky = SceneSetup::skyColor(mNight);
	glClearColor(sky.r, sky.g, sky.b, sky.a);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glfwSwapBuffers(mWindow);
	glfwSetWindowTitle(mWindow, (std::string(APP_TITLE) + " - Loading...").c_str());

	return true;
}

//-----------------------------------------------------------------------------
// OpenGL objects are deleted while the context still exists, then the window
//-----------------------------------------------------------------------------
void Application::shutdown()
{
	if (mWindow == NULL)
		return;

	mParticles.releaseGraphics();
	mHDR.reset();
	mSkyLight.reset();
	mPointStatic.clear();
	mPointDynamic.clear();
	mSpotShadow.reset();
	mCharacterShadow.reset();
	mMapShadow.reset();
	mScene.objects.clear();
	mResources.reset();

	glfwDestroyWindow(mWindow);
	mWindow = NULL;
	glfwTerminate();
}

//-----------------------------------------------------------------------------
// Main loop
//-----------------------------------------------------------------------------
void Application::run()
{
	double lastTime = glfwGetTime();

	while (!glfwWindowShouldClose(mWindow))
	{
		glfwPollEvents();

		double currentTime = glfwGetTime();
		update((float)(currentTime - lastTime));
		lastTime = currentTime;

		render();
		showFPS();
		glfwSwapBuffers(mWindow);
	}
}

//-----------------------------------------------------------------------------
// Camera, animation, lights and snow
//-----------------------------------------------------------------------------
void Application::update(float deltaTime)
{
	deltaTime = std::min(deltaTime, MAX_FRAME_TIME);

	// The free camera always moves, even when the world is frozen
	updateInput(deltaTime);

	// Time of the world: stopped when frozen (key T)
	float worldTime = mFrozen ? 0.0f : deltaTime;

	// Camera tour: at the end it stays on its last shot (Trump dancing)
	// until C gives the free camera back
	if (mTourMode)
		mTour.update(worldTime);

	// Animated characters (Zed and the dancers)
	if (mAnimationOn)
	{
		for (SceneObject& object : mScene.objects)
		{
			if (object.animated && object.skinned != nullptr)
				object.skinned->update(worldTime);
		}
	}

	// The flashlight follows the camera, a little below the eyes
	const Camera& camera = activeCamera();
	SpotLight& flashlight = mLights.getSpotLight();
	flashlight.position = camera.getPosition() - glm::vec3(0.0f, 0.5f, 0.0f);
	flashlight.direction = camera.getLook();

	// Snow falls around the camera
	if (mSnowOn)
		mSnow.update(worldTime, camera.getPosition());
	mParticles.update(worldTime);
}

//-----------------------------------------------------------------------------
// Mouse and keyboard (free camera only)
//-----------------------------------------------------------------------------
void Application::updateInput(float deltaTime)
{
	double mouseX, mouseY;
	glfwGetCursorPos(mWindow, &mouseX, &mouseY);
	if (mFirstMouse)
	{
		mLastMouseX = mouseX;
		mLastMouseY = mouseY;
		mFirstMouse = false;
	}
	double deltaX = mLastMouseX - mouseX;
	double deltaY = mLastMouseY - mouseY;
	mLastMouseX = mouseX;
	mLastMouseY = mouseY;

	if (mTourMode)
		return;

	// Rotate the camera by how much the mouse moved since the last frame
	if (glfwGetWindowAttrib(mWindow, GLFW_FOCUSED))
		mFPSCamera.rotate((float)deltaX * MOUSE_SENSITIVITY, (float)deltaY * MOUSE_SENSITIVITY);

	// Look with the arrow keys too
	float turn = TURN_SPEED * deltaTime;
	if (glfwGetKey(mWindow, GLFW_KEY_LEFT) == GLFW_PRESS)
		mFPSCamera.rotate(turn, 0.0f);
	else if (glfwGetKey(mWindow, GLFW_KEY_RIGHT) == GLFW_PRESS)
		mFPSCamera.rotate(-turn, 0.0f);
	if (glfwGetKey(mWindow, GLFW_KEY_UP) == GLFW_PRESS)
		mFPSCamera.rotate(0.0f, turn);
	else if (glfwGetKey(mWindow, GLFW_KEY_DOWN) == GLFW_PRESS)
		mFPSCamera.rotate(0.0f, -turn);

	float speed = MOVE_SPEED * deltaTime;
	if (glfwGetKey(mWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
		speed *= RUN_FACTOR;

	// Forward/backward
	if (glfwGetKey(mWindow, GLFW_KEY_W) == GLFW_PRESS)
		mFPSCamera.move(speed * mFPSCamera.getLook());
	else if (glfwGetKey(mWindow, GLFW_KEY_S) == GLFW_PRESS)
		mFPSCamera.move(speed * -mFPSCamera.getLook());

	// Strafe left/right
	if (glfwGetKey(mWindow, GLFW_KEY_A) == GLFW_PRESS)
		mFPSCamera.move(speed * -mFPSCamera.getRight());
	else if (glfwGetKey(mWindow, GLFW_KEY_D) == GLFW_PRESS)
		mFPSCamera.move(speed * mFPSCamera.getRight());

	// Up/down
	if (glfwGetKey(mWindow, GLFW_KEY_SPACE) == GLFW_PRESS)
		mFPSCamera.move(speed * glm::vec3(0.0f, 1.0f, 0.0f));
	else if (glfwGetKey(mWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
		mFPSCamera.move(speed * -glm::vec3(0.0f, 1.0f, 0.0f));
}

//-----------------------------------------------------------------------------
// The camera used to draw: the tour camera during the tour
//-----------------------------------------------------------------------------
const Camera& Application::activeCamera() const
{
	if (mTourMode)
		return mTourCamera;
	return mFPSCamera;
}

//-----------------------------------------------------------------------------
// 1. shadow maps that change: characters (moon, lamps) and flashlight
// 2. scene, moon and snow into the HDR image  3. bloom and final image
//-----------------------------------------------------------------------------
void Application::render()
{
	int width, height;
	glfwGetFramebufferSize(mWindow, &width, &height);
	if (width <= 0 || height <= 0)
		return;		// minimized

	// 1. Shadows that change every frame
	const SpotLight& flashlight = mLights.getSpotLight();
	bool spotShadow = mShadowsOn && flashlight.on;
	mRenderer.setShadowMaps(mMapShadow.get(), mCharacterShadow.get(), mShadowsOn);
	mRenderer.setSpotShadow(mSpotShadow.get(), spotShadow);
	if (mShadowsOn)
	{
		// Moon: characters in their current pose
		mRenderer.renderShadows(mScene, ShadowGroup::Dynamic, *mCharacterShadow, width, height);

		// Lamps: moving characters near each lamp
		for (size_t i = 0; i < mPointDynamic.size(); i++)
			mRenderer.renderPointShadows(mScene, *mPointDynamic[i], true, SceneSetup::POINT_SHADOW_DYNAMIC_RANGE, width, height);
	}
	if (spotShadow)
	{
		// Flashlight: everything, seen from the flashlight (it moves with the camera)
		mSpotShadow->setSpotLight(flashlight.position, flashlight.direction, SceneSetup::SPOT_SHADOW_FOV,
		                          0.3f, SceneSetup::SPOT_SHADOW_FAR);
		mRenderer.renderSpotShadows(mScene, *mSpotShadow, width, height);
	}

	// 2. Scene
	glm::vec4 sky = SceneSetup::skyColor(mNight);
	glClearColor(sky.r, sky.g, sky.b, sky.a);
	bool hdr = mHDR && mHDR->begin(width, height);
	if (!hdr)
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	const Camera& camera = activeCamera();
	glm::mat4 view = camera.getViewMatrix();
	glm::mat4 projection = glm::perspective(glm::radians(camera.getFOV()), (float)width / (float)height, 0.5f, 2000.0f);

	mRenderer.render(mScene, mLights, view, projection, camera.getPosition());

	// The main light, where its light comes from (hidden by the mountains)
	if (mSkyLight)
		mSkyLight->render(view, projection, camera.getPosition(), mLights.getDirectionalLight().direction,
		                  SceneSetup::skyLightColor(mNight));

	mParticles.render(view, projection);

	// 3. Bloom + final image
	if (hdr)
		mHDR->finish(mBloomOn);
}

//-----------------------------------------------------------------------------
// Average frames per second and time per frame, in the window title
//-----------------------------------------------------------------------------
void Application::showFPS()
{
	mFrameCount++;
	double currentSeconds = glfwGetTime();
	double elapsedSeconds = currentSeconds - mFPSStart;

	// Limit text updates to 4 times per second
	if (elapsedSeconds <= 0.25)
		return;

	double fps = (double)mFrameCount / elapsedSeconds;
	double msPerFrame = 1000.0 / fps;

	std::ostringstream outs;
	outs.precision(3);	// decimal places
	outs << std::fixed
		<< APP_TITLE << "    "
		<< "FPS: " << fps << "    "
		<< "Frame Time: " << msPerFrame << " (ms)";
	if (mTourMode && mTour.isPlaying())
		outs << "    Tour: " << mTour.getTime() << " / " << mTour.getDuration() << " s";
	else if (mTourMode)
		outs << "    Tour: end (C: free camera)";
	if (mMusic.isMuted())
		outs << "    [Muted]";
	if (mFrozen)
		outs << "    [Frozen]";
	glfwSetWindowTitle(mWindow, outs.str().c_str());

	mFPSStart = currentSeconds;
	mFrameCount = 0;
}

//-----------------------------------------------------------------------------
// Prints the keys, grouped, one per line:
//
//   MOVE
//     W A S D ............ walk (Z Q S D on an AZERTY keyboard)
//-----------------------------------------------------------------------------
void Application::printControls() const
{
	struct Control
	{
		const char* key;	// NULL: title of a group
		const char* action;
	};
	const Control CONTROLS[] = {
		{ NULL,                 "MOVE" },
		{ "Mouse / Arrow keys", "look around" },
		{ "W A S D",            "walk (Z Q S D on an AZERTY keyboard)" },
		{ "Space / Left Ctrl",  "go up / down" },
		{ "Left Shift",         "run (hold)" },
		{ "Mouse wheel",        "zoom" },
		{ NULL,                 "VIEW" },
		{ "C",                  "camera tour: start / stop (it ends on Trump)" },
		{ "N",                  "day / night" },
		{ "F",                  "flashlight on / off" },
		{ "O",                  "shadows on / off" },
		{ "G",                  "glow around lights (bloom) on / off" },
		{ "P",                  "snow on / off" },
		{ "F1",                 "wireframe on / off" },
		{ NULL,                 "TIME AND SOUND" },
		{ "B",                  "characters: pause / play their animation" },
		{ "T",                  "freeze everything (characters, snow, tour, music)" },
		{ "M",                  "sound: mute / unmute" },
		{ "Esc",                "quit" }
	};
	const size_t KEY_WIDTH = 20;	// key column, filled with dots

	std::cout << std::endl
		<< "  ======================== " << APP_TITLE << " - Controls ========================" << std::endl;
	for (const Control& control : CONTROLS)
	{
		if (control.key == NULL)
		{
			std::cout << std::endl << "  " << control.action << std::endl;
			continue;
		}

		std::string key = control.key;
		std::cout << "    " << key << " " << std::string(KEY_WIDTH - key.size(), '.') << " " << control.action << std::endl;
	}
	std::cout << std::endl
		<< "  ==========================================================================" << std::endl << std::endl;
}

//-----------------------------------------------------------------------------
// GLFW callbacks -> object methods
//-----------------------------------------------------------------------------
void Application::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
	if (app != NULL)
		app->onKey(key, action);
}

void Application::framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
	if (app != NULL)
		app->onFramebufferSize(width, height);
}

void Application::scrollCallback(GLFWwindow* window, double deltaX, double deltaY)
{
	Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
	if (app != NULL)
		app->onMouseScroll(deltaY);
}

//-----------------------------------------------------------------------------
// Keys
//-----------------------------------------------------------------------------
void Application::onKey(int key, int action)
{
	if (action != GLFW_PRESS)
		return;

	switch (key)
	{
	case GLFW_KEY_ESCAPE:
		glfwSetWindowShouldClose(mWindow, GL_TRUE);
		break;
	case GLFW_KEY_F1:
		mWireframe = !mWireframe;
		glPolygonMode(GL_FRONT_AND_BACK, mWireframe ? GL_LINE : GL_FILL);
		break;
	case GLFW_KEY_F:
		mLights.getSpotLight().on = !mLights.getSpotLight().on;	// flashlight
		break;
	case GLFW_KEY_N:
		mNight = !mNight;
		SceneSetup::setTimeOfDay(mLights, mNight);
		break;
	case GLFW_KEY_O:
		mShadowsOn = !mShadowsOn;
		break;
	case GLFW_KEY_G:
		mBloomOn = !mBloomOn;
		break;
	case GLFW_KEY_P:
		// Snow on / off at once: the sky is filled (or emptied) immediately
		mSnowOn = !mSnowOn;
		mParticles.clear();
		if (mSnowOn)
			mSnow.prefill(activeCamera().getPosition());
		break;
	case GLFW_KEY_B:
		mAnimationOn = !mAnimationOn;
		break;
	case GLFW_KEY_M:
		mMusic.setMuted(!mMusic.isMuted());
		break;
	case GLFW_KEY_T:
		// Freeze / unfreeze everything that moves by itself
		mFrozen = !mFrozen;
		mMusic.setPaused(mFrozen);
		break;
	case GLFW_KEY_C:
		// Start the tour from the beginning, or stop it
		mTourMode = !mTourMode;
		if (mTourMode)
			mTour.restart();
		break;
	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Window resized
//-----------------------------------------------------------------------------
void Application::onFramebufferSize(int width, int height)
{
	glViewport(0, 0, width, height);
}

//-----------------------------------------------------------------------------
// Mouse wheel: zoom (field of view of the free camera)
//-----------------------------------------------------------------------------
void Application::onMouseScroll(double deltaY)
{
	double fov = mFPSCamera.getFOV() + deltaY * ZOOM_SENSITIVITY;
	fov = glm::clamp(fov, 1.0, 120.0);
	mFPSCamera.setFOV((float)fov);
}

