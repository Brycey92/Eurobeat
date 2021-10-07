#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <chrono>
#include <cstdbool>
#include <dirent>
#include <iostream>
#include <random>
#include <vector>

//#include <boost/algorithm/string.hpp>
//#include <boost/filesystem.hpp>
//#include <filesystem>
//#include "poco/directoryiterator.h"

#include <linux/can.h>
#include <linux/can/bcm.h>

#include "portaudio.h"
#include <sndfile.h>

#define DEBUG 1

#define IFACE "can0"
#define NFRAMES 1
#define CAN_PEDALS_ID 0x0329
#define ACCELERATOR_BYTE_INDEX 6 /*starts at 0*/
#define ACCELERATOR_BYTE_MIN 0
#define ACCELERATOR_BYTE_MAX 0xFD
#define ACTIVATION_PERCENT 80
#define DEACTIVATION_PERCENT 60

#define CUT_MUSIC_DIR "cutmusic/"
#define MUSIC_DIR "music/"
#define FRAMES_PER_BUFFER (512)

using namespace std;
//using namespace boost::filesystem;
//using namespace boost::algorithm;
using namespace Poco;

static default_random_engine generator;

typedef struct {
	SNDFILE* file;
	SF_INFO info;
} callback_data;

string getRandFile(vector<string> audioFiles) {
	if(!audioFiles.empty()) {
		uniform_int_distribution<int> distribution(0,audioFiles.size() - 1);
		return audioFiles[distribution(generator)];
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

PaError handlePa(PaError err) {
	if( err < 0/*!= paNoError*/ ) {
		cerr << "PortAudio error: " << Pa_GetErrorText(err) << endl;
		exit(EXIT_FAILURE);
	}
	else {
		return err;
	}
}

int main(int argc, char* argv[]) {
	const uint8_t byteRange = (ACCELERATOR_BYTE_MAX - ACCELERATOR_BYTE_MIN);
	const uint8_t activationValue = byteRange * (ACTIVATION_PERCENT / 100) + ACCELERATOR_BYTE_MIN;
	const uint8_t deactivationValue = byteRange * (DEACTIVATION_PERCENT / 100) + ACCELERATOR_BYTE_MIN;

    struct sockaddr_can addr;
    struct ifreq ifr;
	
	struct can_msg {
        struct bcm_msg_head msg_head;
        struct can_frame frame[NFRAMES];
    } msg;
	
	/* Open the CAN interface */
    sock = socket(PF_CAN, SOCK_DGRAM, CAN_BCM);
    if (sock < 0) {
        perror(PROGNAME ": socket");
        return errno;
    }

    strncpy(ifr.ifr_name, iface, IFNAMSIZ);
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        cerr << "Error in ioctl!\n";
        exit(errno);
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        cerr << "Error connecting to CAN interface!\n";
        exit(errno);
    }

    /* Set socket to non-blocking */
    flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) {
        cerr << "Error in fcntl: F_GETFL!\n";
        exit(errno);
    }

    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
        cerr << "Error in fcntl: F_SETFL!\n";
        exit(errno);
    }
	
	/* Setup code */
    sigval = 0;
    msg.msg_head.opcode  = RX_SETUP;
    msg.msg_head.can_id  = CAN_PEDALS_ID;
    msg.msg_head.flags   = 0;
    msg.msg_head.nframes = 0;
    if (write(sock, &msg, sizeof(msg)) < 0) {
        cerr << "Error in write: RX_SETUP!\n";
        exit(errno);
    }
	
	//srand(time(NULL));
	generator = default_random_engine(chrono::system_clock::now().time_since_epoch().count());
	
	//initialize PortAudio
	handlePa(Pa_Initialize());
	
	PaError errDevCount = Pa_GetDeviceCount();
	if(errDevCount < 0) {
		handlePa(errDevCount);
	}
	
	int numDevices = errDevCount;
	if(numDevices == 0) {
		cerr << "No audio devices detected.\n";
		exit(EXIT_FAILURE);
	}
	
	//keep checking for gas pedal messages and playing eurobeat when appropriate
	while(true) {
		//generate a vector of all audio files
		vector<string> audioFiles;
		//C++17
		/*for (auto &p : filesystem::directory_iterator(CUT_MUSIC_DIR)) {
			if(p.is_regular_file()) {
				audioFiles.push_back(p.path())
				#if DEBUG
				cout << "Adding " << p.path << " to file list.\n";
				#endif
			}
		}*/
		//boost
		/*for(auto p : directory_iterator(CUT_MUSIC_DIR)) {
			if(is_regular_file(p)) {
				audioFiles.push_back(p.path());
				#if DEBUG
				cout << "Adding " << p.path().string() << " to file list.\n";
				#endif
			}
		}*/
		//poco
		for(auto p : DirectoryIterator(CUT_MUSIC_DIR)) {
			if(Path(p).isFile()) {
				audioFiles.push_back(p);
				#if DEBUG
				cout << "Adding " << p << " to file list.\n";
				#endif
			}
		}
		
		//pick a random audio file and open it
		callback_data sfData;
		memset(&sfData, 0, sizeof(callback_data));
		
		string filePathStr = CUT_MUSIC_DIR;
		filePathStr += /*"dejavu.wav"*/getRandFile(audioFiles);
		
		sfData.file = sf_open(filePathStr.c_str(), SFM_READ, &sfData.info) ;
		if (sf_error(sfData.file) != SF_ERR_NO_ERROR) {
			cerr << sf_strerror(sfData.file) << endl;
			exit(EXIT_FAILURE);
		}
		
		//begin opening PortAudio stream
		PaStream *stream = NULL;
		
		//if we have multiple audio devices, get their info
		//const PaDeviceInfo *deviceInfo;
		if(numDevices > 1) {
			//structs for checking device compatibility with the audio file
			PaStreamParameters outputParameters;
			//memset(&outputParameters, 0, sizeof(PaStreamParameters));
			
			outputParameters.channelCount = sfData.info.channels; /* stereo output */
			outputParameters.sampleFormat = paFloat32; 			  /* 32 bit floating point output */
			outputParameters.hostApiSpecificStreamInfo = NULL;	  /* See your specific host's API docs
																  for info on using this field */
			
			//check each device's info
			for(int i = 0; numDevices > 1 && stream == NULL && i < numDevices; i++) {
				string name = Pa_GetDeviceInfo(i)->name;
				#if DEBUG
				cout << "Found audio device: " << name << endl;
				#endif
				
				to_lower(name);
				#if DEBUG
				cout << "Lowercase device name: " << name << endl;
				#endif
				
				//check if the current device is a USB device
				if(name.find("usb") != std::string::npos) {
					#if DEBUG
					cout << "Trying to open stream on " << name << endl;
					#endif
					
					outputParameters.device = i;
					outputParameters.suggestedLatency = Pa_GetDeviceInfo(i)->defaultLowOutputLatency;
					
					//make sure the device supports the audio file
					PaError err = handlePa(Pa_IsFormatSupported(NULL, &outputParameters, sfData.info.samplerate));
					if(err == paFormatIsSupported) {
						//if everything checks out, open a stream on the current device
						handlePa(Pa_OpenStream( &stream,
												NULL,
												&outputParameters,
												sfData.info.samplerate,
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
												&sfData ));		   /* data to be passed to callback */
					}
					#if DEBUG
					else {
						cout << "Device " << name << "doesn't support the audio file:\n";
						cout << "Channels: " << outputParameters.channelCount << endl;
						cout << "Sample rate: " << sfData.info.samplerate << endl;
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
											sfData.info.channels,/* stereo output */
											paFloat32,  	   /* 32 bit floating point output */
											sfData.info.samplerate,
											FRAMES_PER_BUFFER, /* frames per buffer, i.e. the number
															   of sample frames that PortAudio will
															   request from the callback. Many apps
															   may want to use
															   paFramesPerBufferUnspecified, which
															   tells PortAudio to pick the best,
															   possibly changing, buffer size. */
											paCallback, 	   /* this is your callback function */
											&sfData ));   	   /* data to be passed to callback */
		}
	
		/*while (Can0.available() == 0) {
		}
		
		// Read the received data from CAN0 mailbox 0
		CAN_FRAME incoming;
		Can0.read(incoming);*/
		
		/*if(incoming.length > ACCELERATOR_BYTE_INDEX) {
			if(incoming.data.bytes[ACCELERATOR_BYTE_INDEX] > activationValue) {*/
				//play audio
				handlePa(Pa_StartStream(stream));
				#if DEBUG
				cout << "Playing audio... ";
				#endif
				
				/*if (Can0.available() > 0) {
					Can0.read(incoming); 
				}*/
				
				while(!handlePa(Pa_IsStreamStopped(stream))) {
					Pa_Sleep(50);
					
					//if the stream is no longer active...
					if(!handlePa(Pa_IsStreamActive(stream))) {
						//stop audio immediately by aborting
						if(!handlePa(Pa_IsStreamStopped(stream))) {
							handlePa(Pa_AbortStream(stream));
						}
					}
				}
				
				#if DEBUG
				cout << "done.\n";
				#endif
				
				if(sf_close(sfData.file)) {
					cout << "Failed to close audio file!\n";
					exit(EXIT_FAILURE);
				}
				
				//close PortAudio stream
				handlePa(Pa_CloseStream(stream));
			/*}
		}
		else {
			cout << "Message received was too short!\n Expected at least " << (ACCELERATOR_BYTE_INDEX + 1) << " bytes, but received " << incoming.length << ".\n";
		}*/
	}
	
	//terminate PortAudio
	handlePa(Pa_Terminate());
	
	/* Close the CAN interface */
    if (close(sock) < 0) {
        cerr << "Error closing CAN interface!\n";
        exit(errno);
    }
}
