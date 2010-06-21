/*
 * Mp3Helper.h Helper class to enable playing MP3 sounds in
 *             PSP homebrew applications
 *
 */

#ifndef MP3HELPER_H_
#define MP3HELPER_H_

extern "C"{
#include <stdio.h>
}

class ClMp3Helper {
public:
	static bool initMP3();
	static void closeMP3();
	static int playMP3(const char* fileName, unsigned short volume);
	static void stopMP3(int mp3ID);
	static void setMP3Volume(int mp3ID, unsigned short volume);

private:
	static int threadId;
	static int fillStreamBuffer(FILE* fileHandle, int mp3Handle);
	static int playThread(SceSize args, void *argp);

protected:
	ClMp3Helper();
	virtual ~ClMp3Helper();
};

#endif /* MP3HELPER_H_ */
