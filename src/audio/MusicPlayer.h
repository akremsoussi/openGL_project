//-----------------------------------------------------------------------------
// Plays a sound file (MP3, WAV, FLAC) in a loop with miniaudio, with mute.
// If there is no sound device the program simply runs without sound.
//-----------------------------------------------------------------------------
#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H

#include <memory>
#include <string>

class MusicPlayer
{
public:
	 MusicPlayer();
	~MusicPlayer();

	// Opens the sound device and starts playing the file in a loop
	bool play(const std::string& filename);

	void setMuted(bool muted);
	bool isMuted() const { return mMuted; }

	void setPaused(bool paused);	// resumes where it stopped
	bool isReady() const { return mReady; }

private:
	MusicPlayer(const MusicPlayer& rhs);
	MusicPlayer& operator = (const MusicPlayer& rhs);

	// miniaudio objects, hidden here so that only MusicPlayer.cpp includes miniaudio.h
	struct Audio;
	std::unique_ptr<Audio> mAudio;
	bool mReady;
	bool mMuted;
};
#endif //MUSICPLAYER_H
