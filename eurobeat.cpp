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

#define AUDIO_TEST_ONLY 1

#define CUT_MUSIC_DIR "cutmusic/"
#define MUSIC_DIR "music/"

using namespace std;

int main(int argc, char* argv[]) {
	const uint8_t byteRange = (ACCELERATOR_BYTE_MAX - ACCELERATOR_BYTE_MIN);
	const uint8_t activationValue = byteRange * (ACTIVATION_PERCENT / 100) + ACCELERATOR_BYTE_MIN;
	const uint8_t deactivationValue = byteRange * (DEACTIVATION_PERCENT / 100) + ACCELERATOR_BYTE_MIN;

	int sock;
	can_msg msg;
	
	#if !AUDIO_TEST_ONLY
	sock = openCan();
	#endif
	
	//initialize PortAudio
	handlePa(Pa_Initialize());
	
	//keep checking for gas pedal messages and playing eurobeat when appropriate
	while(sigval == 0) {
		//generate a vector of all audio files
		vector<string> audioFiles;
		
		//pick a random audio file and open it
		callback_data sfData;
		memset(&sfData, 0, sizeof(callback_data));
		
		string filePathStr = CUT_MUSIC_DIR;
		filePathStr += "dejavu.wav";//getRandFile(audioFiles);
		
		if(!openAudioFile(filePathStr, &sfData)) {
			exit(EXIT_FAILURE);
		}
		
		//open a PortAudio stream
		PaStream *stream;
		openPaStream(&sfData, stream);
		
		struct can_frame* frame;
		
		bool playedFile = false;
		
		//keep reading frames until the gas pedal is pressed far enough
		while(!playedFile) {
			#if !AUDIO_TEST_ONLY
			#if DEBUG
			cout << "Waiting for CAN messages.";
			#endif
			
			/* Read from the CAN interface */
			while(!readCan(frame, sock)) {
				#if DEBUG
				cout << ".";
				#endif
			}
			
			//check the frame's data
			if(frame->can_dlc > ACCELERATOR_BYTE_INDEX) {
				if(frame->data[ACCELERATOR_BYTE_INDEX] > activationValue) {
			#endif
					//play audio
					handlePa(Pa_StartStream(stream));
					playedFile = true;
					#if DEBUG
					cout << "Playing audio... ";
					#endif
					
					//stay in this loop while the stream is not stopped
					while(!handlePa(Pa_IsStreamStopped(stream))) {
						//Pa_Sleep(50);
						
						//stay in this loop while there is still audio left to play in the file
						while(handlePa(Pa_IsStreamActive(stream))) {
							/* Read from the CAN interface */
							#if !AUDIO_TEST_ONLY
							while(!readCan(frame, sock)) {}
							
							//check the frame's data
							if(frame->can_dlc > ACCELERATOR_BYTE_INDEX) {
								if(frame->data[ACCELERATOR_BYTE_INDEX] < deactivationValue) {
									//stop audio immediately by aborting
									if(!handlePa(Pa_IsStreamStopped(stream))) {
										handlePa(Pa_AbortStream(stream));
										#if DEBUG
										cout << "stopped due to gas pedal position.\n";
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
								cout << "done.\n";
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
	}
	
	//terminate PortAudio
	handlePa(Pa_Terminate());
	
	/* Close the CAN interface */
	#if !AUDIO_TEST_ONLY
    closeCan(sock);
	#endif
}
