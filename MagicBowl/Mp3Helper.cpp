/*
 * Mp3Helper.cpp
 *
 *  Created on: Feb 10, 2010
 *      Author: andreborrmann
 */

extern "C"{
#include <pspkernel.h>
#include <pspaudio.h>
#include <pspmp3.h>
#include <psputility.h>
//#include <pspdebug.h>
#include <stdlib.h>
#include <malloc.h>
}

#include "Mp3Helper.h"

/*
char	mp3Buf[16*1024]  __attribute__((aligned(64)));
short	pcmBuf[16*(1152/2)]  __attribute__((aligned(64)));
*/
/*
 * Bits send to the MP3 thread to get control from outside
 * they are or'd together
 * additional attributes will be stored at bit position 16 to 32
 */
enum Mp3ControlBits {
	MP3_STOP_BIT       = 0b00001,
	MP3_SET_VOLUME_BIT = 0b00010,
	MP3_ALL_BITS       = 0xffffffff
};

typedef struct ThreadParams{
	const char* fileName;
	int eventId;
	unsigned short volume;
}ThreadParams;

int ClMp3Helper::threadId = 0;

ClMp3Helper::ClMp3Helper() {
	// TODO Auto-generated constructor stub

}

int ClMp3Helper::fillStreamBuffer(FILE* fileHandle, int mp3Handle){
	unsigned char* destination;
	int bytesToWrite;
	int srcPos;

	int mp3Status;
	int bytesRead;

	//this will get the amount of data needed to be red and the
	//memory will be allocated accordingly
	mp3Status = sceMp3GetInfoToAddStreamData(mp3Handle, &destination, &bytesToWrite, &srcPos);
	if (mp3Status < 0) return -1;
	//go to the file position requested
	mp3Status = fseek( fileHandle, srcPos, SEEK_SET );
	if (mp3Status < 0) return -1;
	//now read the data requested
	bytesRead = fread( destination, sizeof(char), bytesToWrite, fileHandle );
	if (bytesRead <0) return -1;
	if (bytesRead == 0) return 0; //reached end of file

	//notify MP3 lib how many bytes we have processed
	mp3Status = sceMp3NotifyAddStreamData( mp3Handle, bytesRead );
	if (mp3Status <0) return -1;

	return (srcPos > 0);
}

bool ClMp3Helper::initMP3(){
	//load the modules for the codecs
	int status = sceUtilityLoadModule(PSP_MODULE_AV_AVCODEC);
	if (status < 0) return false;
	status = sceUtilityLoadModule(PSP_MODULE_AV_MP3);
	if (status < 0) return false;
	status = sceMp3InitResource();
	if (status < 0) return false;

	return true;
}


int ClMp3Helper::playMP3(const char *fileName, unsigned short volume){

	//now start playing in a separate thread to allow the main application
	//continue running
	threadId = sceKernelCreateThread("playSound", ClMp3Helper::playThread, 0x11, 0x10000, THREAD_ATTR_USER, 0);
	if (threadId >= 0){
		ThreadParams params;
		params.fileName = fileName;
		//to allow control of the thread from outside
		//we create a EventFlag the thread is waiting for
		//the method will return the ID of this EventFlag
		SceUID threadEvent = sceKernelCreateEventFlag(fileName, 0,0,0);//PSP_EVENT_WAITMULTIPLE, 0, 0);
		params.eventId = threadEvent;
		params.volume = volume;
		sceKernelStartThread(threadId, sizeof(params), &params);

		return threadEvent;
	}

	return -1;
}

void ClMp3Helper::setMP3Volume(int mp3ID, unsigned short volume){
	sceKernelSetEventFlag(mp3ID, MP3_SET_VOLUME_BIT | volume << 16);
}


void ClMp3Helper::stopMP3(int mp3ID){
	sceKernelSetEventFlag(mp3ID, MP3_STOP_BIT);
}

int ClMp3Helper::playThread(SceSize args, void *argp){

	int status;

	ThreadParams* params = (ThreadParams*)argp;
	FILE* fileHandle = fopen( params->fileName, "rb" );
	//int fd = sceIoOpen( params->fileName, PSP_O_RDONLY, 0777 );

	if (!fileHandle) return -1;
	//if (fd < 0) return -1;

	SceMp3InitArg mp3Init;
	mp3Init.mp3StreamStart = 0;
	fseek( fileHandle, 0, SEEK_END );
	mp3Init.mp3StreamEnd = ftell(fileHandle);
	mp3Init.unk1 = 0;
	mp3Init.unk2 = 0;
	mp3Init.mp3Buf = memalign(64, 16*1024*sizeof(char));
	mp3Init.mp3BufSize = 16*1024*sizeof(char);
	mp3Init.pcmBuf = memalign(64, 16*(1152/2)*sizeof(short));
	mp3Init.pcmBufSize = 16*(1152/2)*sizeof(short);

	//get the MP3 handle
	int mp3Handle = sceMp3ReserveMp3Handle( &mp3Init );
	//fill the stream Buffer
	ClMp3Helper::fillStreamBuffer(fileHandle, mp3Handle);

	sceMp3Init( mp3Handle );

	//int samplingRate = sceMp3GetSamplingRate( mp3Handle );
	int numChannels = sceMp3GetMp3ChannelNum( mp3Handle );
	//int kbits = sceMp3GetBitRate( mp3Handle );
	int lastDecoded = 0;
	int volume = params->volume;//PSP_AUDIO_VOLUME_MAX;
	bool playing = true;
	int channel = -1;

	while (playing) {

		//first check if we need new data
		// Check if we need to fill our stream buffer
		if (sceMp3CheckStreamDataNeeded( mp3Handle )>0)
		{
			//if there was nothing more read from the buffer finish playing
			if(!ClMp3Helper::fillStreamBuffer( fileHandle, mp3Handle ))
				playing = false;
		}
		//no wait for the EventFlag
		unsigned int eventBits = 0;
		unsigned int waitTime = 100;
		sceKernelWaitEventFlag(params->eventId, MP3_ALL_BITS, PSP_EVENT_WAITAND | PSP_EVENT_WAITCLEAR, &eventBits, &waitTime);
		if (eventBits & MP3_STOP_BIT){
			//we have recieved stop flag
			playing = false;
		}
		if (eventBits & MP3_SET_VOLUME_BIT){
			volume = eventBits >> 16;
			sceKernelSetEventFlag(params->eventId, 0);
		}
		// Decode some samples
		short* buf;
		int bytesDecoded;
		int retries = 0;

		bytesDecoded = sceMp3Decode( mp3Handle, &buf );
		//error in decoding stops playing
		if (bytesDecoded<0 && bytesDecoded!=0x80671402){
			playing = false;
		}
		//if there is nothing more to decode, we have finished
		if (bytesDecoded==0 || bytesDecoded==0x80671402)
		{
			playing = false;
		} else {
			// Reserve the Audio channel for our output if not yet done
			if (channel<0 || lastDecoded!=bytesDecoded)
			{
				if (channel>=0)
					//sceAudioSRCChRelease();
					sceAudioChRelease(channel);

				//channel = sceAudioSRCChReserve( bytesDecoded/(2*numChannels), samplingRate, numChannels );
				channel = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, PSP_AUDIO_SAMPLE_ALIGN(bytesDecoded/(2*numChannels)), PSP_AUDIO_FORMAT_STEREO);

				//sceKernelDelayThread(10000);
				lastDecoded = bytesDecoded;
			}
			// Output the decoded sample
			//sceAudioSRCOutputBlocking( volume, buf );
			status = sceAudioOutputBlocking(channel, volume, buf);
		}
		sceKernelDelayThread(10000);
	}

	//if we have finished playing...some cleanup
	if (channel>=0)
		//sceAudioSRCChRelease();
		sceAudioChRelease(channel);

	sceMp3ReleaseMp3Handle( mp3Handle );
	fclose( fileHandle );
	free(mp3Init.mp3Buf);
	free(mp3Init.pcmBuf);

	threadId=0;
	return sceKernelExitDeleteThread(0);
}

void ClMp3Helper::closeMP3(){

	if (threadId){
		//the file is still playing...
		//kill the play thread and close the file
		sceKernelTerminateThread(threadId);
		//fclose(fileHandle);
	}

	sceMp3TermResource();
}

ClMp3Helper::~ClMp3Helper() {
	// TODO Auto-generated destructor stub
}
