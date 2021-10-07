#include "audioutil.h"

#include "eurobeat.h"
#include "util.h"

#include <iostream>
#include <string.h>
#include <unistd.h>

using namespace std;

PaError handlePa(PaError err) {
	if( err < 0/*!= paNoError*/ ) {
		cerr << "PortAudio error: " << Pa_GetErrorText(err) << " (" << err << ")\n";
		exit(err);
	}
	else {
		return err;
	}
}

int paCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,
		const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void *userData) {
	float *out = (float*) outputBuffer;
	(void)inputBuffer;
	(void)timeInfo;
	(void)statusFlags;
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

//if onlyUsb is true, and waitForDevices is false, exit if no USB devices are found
//if onlyUsb is false, and waitForDevices is true, keep checking until any device is found, USB preferred
//if both are true, keep checking until a USB device is found
//if both are false, return any device that's ready, USB preferred, or exit if none are found
bool getPaDevice(callback_data *sfData, PaStreamParameters *outputParameters, bool onlyUsb, bool waitForDevices) {
	memset(outputParameters, 0, sizeof(PaStreamParameters));
	
	//do {
		int numDevices = handlePa(Pa_GetDeviceCount());
		if(numDevices == 0 && !waitForDevices) {
			//numDevices = 0 and waitForDevices = false
			cerr << "No audio devices detected!\n";
			exit(EXIT_FAILURE);
		}
		//if we have multiple audio devices, get their info
		//const PaDeviceInfo *deviceInfo;
		else if(numDevices > 0) {
			outputParameters->channelCount = sfData->info.channels; /* stereo output */
			outputParameters->sampleFormat = paFloat32; 			  /* 32 bit floating point output */
			outputParameters->hostApiSpecificStreamInfo = NULL;	  /* See your specific host's API docs
																  for info on using this field */
			
			//check each device's info
			for(int i = 0; numDevices > 1 && i < numDevices; i++) {
				string name = Pa_GetDeviceInfo(i)->name;
				#if DEBUG
				cout << "Found audio device: " << name << endl;
				#endif
				
				string lowerName = to_lower(name);
				
				//check if the current device is a USB device
				if(lowerName.find("usb") != std::string::npos) {
					#if DEBUG
					cout << "Checking audio file support on " << name << endl;
					#endif
					
					outputParameters->device = i;
					outputParameters->suggestedLatency = Pa_GetDeviceInfo(i)->defaultLowOutputLatency;
					
					//make sure the device supports the audio file
					PaError err = handlePa(Pa_IsFormatSupported(NULL, outputParameters, sfData->info.samplerate));
					if(err == paFormatIsSupported) {
						//if everything checks out, return the current device's outputParameters
						return true;
					}
					#if DEBUG
					else {
						cout << "Device " << name << "doesn't support the audio file:\n";
						cout << "Channels: " << outputParameters->channelCount << endl;
						cout << "Sample rate: " << sfData->info.samplerate << endl;
						cout << "Suggested latency: " << outputParameters->suggestedLatency << endl;
					}
					#endif
				}
			}
			
			//no devices are USB
			if(!onlyUsb) {
				cerr << "No USB audio devices detected. Using default device.\n";
				outputParameters->device = -1;
				return true;
			}
			else if(!waitForDevices) {
				cerr << "No USB audio devices detected!\n";
				exit(EXIT_FAILURE);
			}
		}
		//we're supposed to wait for the list of devices to change, so let the loop run again after 50ms
		#if DEBUG
		cout << "Waiting " << (WAIT_FOR_DEVICES_US / 1000.0) << "ms for audio devices.\n";
		#endif
		usleep(WAIT_FOR_DEVICES_US);
		return false;
	//} while(waitForDevices);
}
