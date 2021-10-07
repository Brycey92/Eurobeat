#include "eurobeat.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <cstdbool>
#include <iostream>
#include <vector>

#include <linux/can.h>

#include "portaudio.h"
#include <sndfile.h>

#include "audioutil.h"
#include "canutil.h"
#include "util.h"

#define CUT_MUSIC_DIR "/home/tc/cutmusic/"
#define MUSIC_DIR "/home/tc/music/"

using namespace std;

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;
	const uint8_t byteRange = (ACCELERATOR_BYTE_MAX - ACCELERATOR_BYTE_MIN);
	const uint8_t activationValue = byteRange * (ACTIVATION_PERCENT / 100.0) + ACCELERATOR_BYTE_MIN;
	const uint8_t deactivationValue = byteRange * (DEACTIVATION_PERCENT / 100.0) + ACCELERATOR_BYTE_MIN;

	int sock;
	
	#if !AUDIO_TEST_ONLY
	sock = openCan();
	#if DEBUG
	cout << "Opened CAN bus.\n";
	#endif
	#endif
	
	//keep checking for gas pedal messages and playing eurobeat when appropriate
	while(sigval == 0) {
		//generate a vector of all audio files
		vector<string> audioFiles;
		enumerateFiles(&audioFiles);
		
		//pick a random audio file and open it
		callback_data sfData;
		memset(&sfData, 0, sizeof(callback_data));
		
		string filePathStr = CUT_MUSIC_DIR;
		filePathStr += getRandFile(audioFiles);
		
		if(!openAudioFile(filePathStr, &sfData)) {
			exit(EXIT_FAILURE);
		}
		
		PaStreamParameters outputParameters;
		
		bool suitableDeviceFound = false;
		
		do {
			//initialize PortAudio
			handlePa(Pa_Initialize());
			
			//find a device to open a stream on
			suitableDeviceFound = getPaDevice(&sfData, &outputParameters, true, true);
			if(!suitableDeviceFound) {
				//terminate PortAudio
				handlePa(Pa_Terminate());
			}
		} while(!suitableDeviceFound);
		
		#if DEBUG
		cout << "Initialized PortAudio and selected an audio device.\n";
		#endif
		
		//open a PortAudio stream
		PaStream *stream;
		if(outputParameters.device >= 0) {
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
		else {
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
		
		#if DEBUG
		cout << "Audio stream opened.\n";
		#endif
		
		struct can_frame* frame = NULL;
		
		bool playedFile = false;
		
		//keep reading frames until the gas pedal is pressed far enough
		while(!playedFile) {
			#if !AUDIO_TEST_ONLY
			#if DEBUG
			cout << "Waiting for CAN messages.";
			cout.flush();
			#endif
			
			/* Read from the CAN interface */
			while(!readCan(&frame, sock)) {
				#if ANNOYING_DEBUG
				cout << ".";
				#endif
			}
			/*#if DEBUG
			cout << "\nRecv ID: " << hex << frame->can_id << ", Data: " << hex << frame->data << endl;
			#endif*/
			
			//check the frame's data
			if(frame->can_dlc > ACCELERATOR_BYTE_INDEX) {
				if(frame->data[ACCELERATOR_BYTE_INDEX] >= activationValue) {
			#endif
					//play audio
					handlePa(Pa_StartStream(stream));
					playedFile = true;
					#if DEBUG
					cout << "Playing audio.\n";
					#endif
					
					//stay in this loop while the stream is not stopped
					while(!handlePa(Pa_IsStreamStopped(stream))) {
						//Pa_Sleep(50);
						
						//stay in this loop while there is still audio left to play in the file
						while(handlePa(Pa_IsStreamActive(stream))) {
							/* Read from the CAN interface */
							#if !AUDIO_TEST_ONLY
							#if DEBUG
							cout << "Waiting for CAN messages,";
							cout.flush();
							#endif
							while(!readCan(&frame, sock)) {
								#if ANNOYING_DEBUG
								cout << ",";
								#endif
							}
							
							/*#if DEBUG
							cout << "\nRecv ID: " << hex << frame->can_id << ", Data: " << hex << frame->data << endl;
							#endif*/
							
							//check the frame's data
							if(frame->can_dlc > ACCELERATOR_BYTE_INDEX) {
								if(frame->data[ACCELERATOR_BYTE_INDEX] < deactivationValue) {
									//stop audio immediately by aborting
									if(!handlePa(Pa_IsStreamStopped(stream))) {
										handlePa(Pa_AbortStream(stream));
										#if DEBUG
										cout << "Stopped audio due to gas pedal position.\n";
										#endif
									}
								}
							}
							else {
								cout << "Message received was too short!\n Expected at least " << (ACCELERATOR_BYTE_INDEX + 1) << " bytes, but received " << frame->can_dlc << ".\n";
							}
							#endif
						}
						
						//if the stream is no longer active...
						//if(!handlePa(Pa_IsStreamActive(stream))) { //this if is commented since its condition will always be true after the above while loop
							//stop the stream
							if(!handlePa(Pa_IsStreamStopped(stream))) {
								handlePa(Pa_StopStream(stream));
								#if DEBUG
								cout << "Done.\n";
								#endif
							}
						//}
					}
					
					if(sf_close(sfData.file)) {
						cout << "Failed to close audio file!\n";
						exit(EXIT_FAILURE);
					}
					
					//close PortAudio stream
					handlePa(Pa_CloseStream(stream));
			#if !AUDIO_TEST_ONLY
				}
			}
			else {
				cout << "Message received was too short!\n Expected at least " << (ACCELERATOR_BYTE_INDEX + 1) << " bytes, but received " << frame->can_dlc << ".\n";
			}
			#endif
		}
		//after the file is played
		
		//terminate PortAudio
		handlePa(Pa_Terminate());
	}
	
	/* Close the CAN interface */
	#if !AUDIO_TEST_ONLY
    closeCan(sock);
	#endif
}
