#ifndef AUDIOUTIL
#define AUDIOUTIL

#include "portaudio.h"
#include <sndfile.h>

#include <cstdbool>
#include <string>

#define FRAMES_PER_BUFFER (512)

typedef struct {
	SNDFILE* file;
	SF_INFO info;
} callback_data;

PaError handlePa(PaError err);

int paCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *userData);

bool openAudioFile(std::string filePathStr, callback_data *sfData);

void openPaStream(callback_data *sfData, PaStream *stream);

#endif