# Eurobeat
Make your car stero play eurobeat when you floor the accelerator.

# HackPSU 2018 Questionnaire

## Inspiration
After watching the anime Initial D, I thought it would be cool to have my car play eurobeat whenever I stepped on the gas pedal hard enough.

## What it does
It reads CAN messages over the OBD2 (OnBoard Diagnostics) port of the car, interprets the messages to find how far the accelerator is pressed, and if it's over 80%, it plays eurobeat through a USB sound card to the car's aux input. When it interprets messages saying the accelerator is less than 50% pressed, it stops the audio.

## How I built it
I first used the Macchina M2 hardware and SavvyCAN software to reverse engineer the CAN messages on my car's OBD2 port. Then, I connected a CAN interface board to a Raspberry Pi Zero and wrote code using the socketcan, portaudio and libsndfile libraries to match the behavior I wanted. I used the TinyCore Linux distribution to keep low boot times so the eurobeat can start quckly after turning the car on.

## Challenges I ran into
TinyCore is a unique Linux distro in that it loads everything to memory upon booting. This required more than the usual linux finesse to get everything working in the operating system. Also, I had to solve issues with all the libraries I used, as well as driver problems.

## Accomplishments that I'm proud of
I'm proud of building the audio system and getting TinyCore and the CAN interface working.

Original response: ~~Thus far, I haven't tested the setup on my car itself, but I have the audio subsystem tested and working, including the Pi software, the car's hardware, and the interface between them.~~

## What I learned
I learned a lot about the CAN bus, linux, TinyCore, and audio processing.

## What's next for Eurobeat
I plan to fix the code to pick a random song from a folder of audio files, and add more edited audio files. I also need to test the code on my current vehicle, as I no longer have the car I had in 2018. Finally, I plan on refining the hardware setup and packaging it for sale on retail platforms such as Etsy, eBay, and local Facebook groups.

Original Response: ~~Next, I plan to test the project while connected to my car's OBD2 port. When I get this working in the intended way, I'm going to add code to the program to pick a random song from a folder of edited eurobeat audio files each time the pedal is pressed beyond 80%. Finally, I plan on refining the hardware setup and packaging it for sale on retail platforms such as Etsy, eBay, and local Facebook groups.~~
