//-----------------------------------------------------------------------------
// The application: owns the window, the cameras and the scene, and runs the
// loop  input -> update -> render.
//-----------------------------------------------------------------------------
#ifndef APPLICATION_H
#define APPLICATION_H

#include <memory>
#include <vector>
#include "Camera.h"
#include "animation/Timeline.h"
#include "audio/MusicPlayer.h"
#include "camera/CinematicCamera.h"
#include "graphics/HDRPipeline.h"
#include "graphics/Renderer.h"
#include "graphics/ShadowMap.h"
#include "graphics/PointShadowMap.h"
#include "graphics/SkyLight.h"
#include "scene/LightManager.h"
#include "scene/Scene.h"
#include "scene/SceneResources.h"
#include "systems/ParticleSystem.h"
#include "systems/SnowEmitter.h"

struct GLFWwindow;

class Application
{
public:
	 Application();
	~Application();

	bool init();	// window, OpenGL, scene.  False on error.
	void run();		// until the window is closed

private:
	Application(const Application& rhs);
	Application& operator = (const Application& rhs);

	bool initOpenGL();
	void shutdown();

	void update(float deltaTime);
	void updateInput(float deltaTime);
	void render();
	void showFPS();
	void printControls() const;		// list of the keys, in the terminal
	const Camera& activeCamera() const;

	// GLFW callbacks (static: GLFW calls C functions; they forward to the object)
	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
	static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
	static void scrollCallback(GLFWwindow* window, double deltaX, double deltaY);
	void onKey(int key, int action);
	void onFramebufferSize(int width, int height);
	void onMouseScroll(double deltaY);

	// Window
	GLFWwindow* mWindow;
	int mWindowWidth, mWindowHeight;
	bool mWSL;				// running in WSL (Windows Subsystem for Linux)

	// Options (keys)
	bool mWireframe, mNight, mShadowsOn, mBloomOn, mSnowOn, mAnimationOn;
	bool mFrozen;			// time stopped: animations, snow, tour and music (the camera still moves)

	// Cameras: free first person camera, or the scripted tour
	FPSCamera mFPSCamera;
	CinematicCamera mTourCamera;
	Timeline mTour;
	bool mTourMode;
	double mLastMouseX, mLastMouseY;
	bool mFirstMouse;

	// FPS in the title
	double mFPSStart;
	int mFrameCount;

	// Scene
	std::unique_ptr<SceneResources> mResources;
	Scene mScene;
	LightManager mLights;
	Renderer mRenderer;
	std::unique_ptr<ShadowMap> mMapShadow;
	std::unique_ptr<ShadowMap> mCharacterShadow;
	std::unique_ptr<ShadowMap> mSpotShadow;		// flashlight
	std::vector<std::unique_ptr<PointShadowMap> > mPointStatic;		// lamps: map and still objects
	std::vector<std::unique_ptr<PointShadowMap> > mPointDynamic;	// lamps: moving characters
	std::unique_ptr<SkyLight> mSkyLight;		// moon / sun disk
	std::unique_ptr<HDRPipeline> mHDR;
	ParticleSystem mParticles;
	SnowEmitter mSnow;

	// Sound
	MusicPlayer mMusic;
};
#endif //APPLICATION_H
