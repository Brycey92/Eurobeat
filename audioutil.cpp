#include "audioutil.h"

#include "eurobeat.h"
#include "util.h"

#include <iostream>
#include <string.h>

using namespace std;

PaError handlePa(PaError err) {
	if( err < 0/*!= paNoError*/ ) {
		cerr << "PortAudio error: " << Pa_GetErrorText(err) << endl;
		exit(EXIT_FAILURE);
	}
	else {
		return err;
	}
}

int paCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *userData) {
	float *out = (float*) outputBuffer;
    callback_data *data = (callback_data*)userData;

    /* clear output buffer */
    memset(out, 0, sizeof(float) * framesPerBuffer * data->info.channels);

    /* read directly into output buffer */
    sf_count_t num_read = sf_read_float(data->file, out, framesPerBuffer * data->info.channels);
    
    /*  If we couldn't read a full frameCount of samples we've reached EOF */
    if (num_read < framesPerBuffer)
    {
        return paComplete;
    }
    
    return paContinue;
}

bool openAudioFile(string filePathStr, callback_data *sfData) {
	sfData->file = sf_open(filePathStr.c_str(), SFM_READ, &(sfData->info)) ;
	if (sf_error(sfData->file) != SF_ERR_NO_ERROR) {
		cerr << sf_strerror(sfData->file) << endl;
		return false;
	}
	else {
		return true;
	}
}

void openPaStream(callback_data *sfData, PaStream *stream) {
	int numDevices = handlePa(Pa_GetDeviceCount());
	
	if(numDevices == 0) {
		cerr << "No audio devices detected.\n";
		exit(EXIT_FAILURE);
	}
	
	//begin opening PortAudio stream
	stream = NULL;
	
	//if we have multiple audio devices, get their info
	//const PaDeviceInfo *deviceInfo;
	if(numDevices > 1) {
		//structs for checking device compatibility with the audio file
		PaStreamParameters outputParameters;
		//memset(&outputParameters, 0, sizeof(PaStreamParameters));
		
		outputParameters.channelCount = sfData->info.channels; /* stereo output */
		outputParameters.sampleFormat = paFloat32; 			  /* 32 bit floating point output */
		outputParameters.hostApiSpecificStreamInfo = NULL;	  /* See your specific host's API docs
															  for info on using this field */
		
		//check each device's info
		for(int i = 0; numDevices > 1 && stream == NULL && i < numDevices; i++) {
			string name = Pa_GetDeviceInfo(i)->name;
			#if DEBUG
			cout << "Found audio device: " << name << endl;
			#endif
			
			string lowerName = to_lower(name);
			#if DEBUG
			cout << "Lowercase device name: " << lowerName << endl;
			#endif
			
			//check if the current device is a USB device
			if(lowerName.find("usb") != std::string::npos) {
				#if DEBUG
				cout << "Trying to open stream on " << name << endl;
				#endif
				
				outputParameters.device = i;
				outputParameters.suggestedLatency = Pa_GetDeviceInfo(i)->defaultLowOutputLatency;
				
				//make sure the device supports the audio file
				PaError err = handlePa(Pa_IsFormatSupported(NULL, &outputParameters, sfData->info.samplerate));
				if(err == paFormatIsSupported) {
					//if everything checks out, open a stream on the current device
					handlePa(Pa_OpenStream( &stream,
											NULL,
											&outputParameters,
											sfData->info.samplerate,
											FRAMES_PER_BUFFER, /* frames per buffer, i.e. the number
															   of sample frames that PortAudio will
															   request from the callback. Many apps
															   may want to use
															   paFramesPerBufferUnspecified, which
															   tells PortAudio to pick the best,
															   possibly changing, buffer size. */
											paNoFlag, 		   /* flags that can be used to define
															   dither, clip settings and more */
											paCallback, 	   /* this is your callback function */
											sfData ));		   /* data to be passed to callback */
				}
				#if DEBUG
				else {
					cout << "Device " << name << "doesn't support the audio file:\n";
					cout << "Channels: " << outputParameters.channelCount << endl;
					cout << "Sample rate: " << sfData->info.samplerate << endl;
					cout << "Suggested latency: " << outputParameters.suggestedLatency << endl;
				}
				#endif
			}
		}
	}
	
	//if no stream was opened...
	if(stream == NULL) {
		//we either have multiple devices, none of which are USB...
		if(numDevices > 1) {
			cerr << "No USB audio devices detected. Using default device.\n";
		}
		
		//or, we have one device. either way, we open a stream on the default device
		handlePa(Pa_OpenDefaultStream( &stream,
										0,          	   /* no input channels */
										sfData->info.channels,/* stereo output */
										paFloat32,  	   /* 32 bit floating point output */
										sfData->info.samplerate,
										FRAMES_PER_BUFFER, /* frames per buffer, i.e. the number
														   of sample frames that PortAudio will
														   request from the callback. Many apps
														   may want to use
														   paFramesPerBufferUnspecified, which
														   tells PortAudio to pick the best,
														   possibly changing, buffer size. */
										paCallback, 	   /* this is your callback function */
										sfData ));   	   /* data to be passed to callback */
	}
}
