#ifndef AUDIOUTIL
#define AUDIOUTIL

#include "portaudio.h"
#include <sndfile.h>

#include <cstdbool>
#include <string>

#define FRAMES_PER_BUFFER (512)
#define WAIT_FOR_DEVICES_US 50000;

typedef struct {
	SNDFILE* file;
	SF_INFO info;
} callback_data;

PaError handlePa(PaError err);

int paCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *userData);

bool openAudioFile(std::string filePathStr, callback_data *sfData);

void getPaDevice(callback_data *sfData, PaStreamParameters *outputParameters, bool onlyUsb, bool waitForDevices);

#endif