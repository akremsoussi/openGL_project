//-----------------------------------------------------------------------------
// Plays a sound file in a loop with miniaudio, with mute.
// miniaudio decodes the MP3 and sends it to the system sound server
// (PulseAudio / PipeWire / ALSA, found when the program starts).
//-----------------------------------------------------------------------------
#include "audio/MusicPlayer.h"
#include <iostream>

#define MA_NO_ENCODING
#define MA_NO_GENERATION
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"

struct MusicPlayer::Audio
{
	ma_engine engine;		// sound device + mixer
	ma_sound sound;			// the music
	bool engineReady = false;
	bool soundReady = false;
};

//-----------------------------------------------------------------------------
// Constructor / destructor
//-----------------------------------------------------------------------------
MusicPlayer::MusicPlayer()
	: mAudio(new Audio()), mReady(false), mMuted(false)
{
}

MusicPlayer::~MusicPlayer()
{
	if (mAudio->soundReady)
		ma_sound_uninit(&mAudio->sound);
	if (mAudio->engineReady)
		ma_engine_uninit(&mAudio->engine);
}

//-----------------------------------------------------------------------------
// Opens the sound device, loads the file and plays it in a loop
//-----------------------------------------------------------------------------
bool MusicPlayer::play(const std::string& filename)
{
	// No sound library found: miniaudio falls back to its silent "Null" backend
	bool noDevice = (ma_engine_init(NULL, &mAudio->engine) != MA_SUCCESS);
	if (!noDevice && ma_engine_get_device(&mAudio->engine)->pContext->backend == ma_backend_null)
	{
		ma_engine_uninit(&mAudio->engine);
		noDevice = true;
	}
	if (noDevice)
	{
		std::cerr << "No sound device: running without sound"
			<< " (on WSL: sudo apt install libpulse0)" << std::endl;
		return false;
	}
	mAudio->engineReady = true;

	// DECODE: the whole file is decoded in memory at load (it is short)
	if (ma_sound_init_from_file(&mAudio->engine, filename.c_str(), MA_SOUND_FLAG_DECODE, NULL, NULL,
	                            &mAudio->sound) != MA_SUCCESS)
	{
		std::cerr << "Cannot load sound " << filename << std::endl;
		return false;
	}
	mAudio->soundReady = true;

	ma_sound_set_looping(&mAudio->sound, MA_TRUE);
	ma_sound_start(&mAudio->sound);
	mReady = true;

	std::cout << "Playing " << filename << " (" << ma_get_backend_name(ma_engine_get_device(&mAudio->engine)->pContext->backend)
		<< "), M = mute" << std::endl;
	return true;
}

//-----------------------------------------------------------------------------
// Pause: stop keeps the play position, start continues from it
//-----------------------------------------------------------------------------
void MusicPlayer::setPaused(bool paused)
{
	if (!mReady)
		return;

	if (paused)
		ma_sound_stop(&mAudio->sound);
	else
		ma_sound_start(&mAudio->sound);
}

//-----------------------------------------------------------------------------
// Mute keeps the music running (silently), so it continues where it is
//-----------------------------------------------------------------------------
void MusicPlayer::setMuted(bool muted)
{
	mMuted = muted;
	if (mAudio->engineReady)
		ma_engine_set_volume(&mAudio->engine, muted ? 0.0f : 1.0f);
}
