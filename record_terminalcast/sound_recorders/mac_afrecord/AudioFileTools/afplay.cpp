/*	Copyright © 2007 Apple Inc. All Rights Reserved.
	
	Disclaimer: IMPORTANT:  This Apple software is supplied to you by 
			Apple Inc. ("Apple") in consideration of your agreement to the
			following terms, and your use, installation, modification or
			redistribution of this Apple software constitutes acceptance of these
			terms.  If you do not agree with these terms, please do not use,
			install, modify or redistribute this Apple software.
			
			In consideration of your agreement to abide by the following terms, and
			subject to these terms, Apple grants you a personal, non-exclusive
			license, under Apple's copyrights in this original Apple software (the
			"Apple Software"), to use, reproduce, modify and redistribute the Apple
			Software, with or without modifications, in source and/or binary forms;
			provided that if you redistribute the Apple Software in its entirety and
			without modifications, you must retain this notice and the following
			text and disclaimers in all such redistributions of the Apple Software. 
			Neither the name, trademarks, service marks or logos of Apple Inc. 
			may be used to endorse or promote products derived from the Apple
			Software without specific prior written permission from Apple.  Except
			as expressly stated in this notice, no other rights or licenses, express
			or implied, are granted by Apple herein, including but not limited to
			any patent rights that may be infringed by your derivative works or by
			other works in which the Apple Software may be incorporated.
			
			The Apple Software is provided by Apple on an "AS IS" basis.  APPLE
			MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
			THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
			FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND
			OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
			
			IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
			OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
			SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
			INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
			MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED
			AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
			STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
			POSSIBILITY OF SUCH DAMAGE.
*/
#include "CAChannelMappingPlayer.h"
#include "CAXException.h"

#if TARGET_OS_MAC
	#include <unistd.h>
#else
	#include <QTML.h>
	#include "CAWindows.h"
#endif

static void usage()
{
	fprintf(stderr,
			"Usage:\n"
			"%s [option...] audio_file\n\n"
			"Options: (may appear before or after arguments)\n"
			"  {-a --all}\n"
			"    for files with >2 channels, play each pair of channels in succession\n"
			"  {-f --frame} FRAME\n"
			"    starting sample frame number\n"
			"  {-s --seconds} START_SECONDS END_SECONDS\n"
			"    segment of file to play, in seconds\n"
			"  {-h --help}\n"
			"    prints this usage summary\n"
			"  {-t --track} TRACK\n"
			"    play the specified track from a multi-track file"
			"  {-v --verbose}"
			"    print out the data format of the file being played"
#if !TARGET_OS_WIN32
			"  {-d --device} AUDIO_DEVICE_NAME\n"
			"    play to the specified audio device\n"
			, getprogname());
#else
			, "afplay");
#endif
	exit(1);
}

static void Play(CAChannelMappingPlayer &player, SInt64 startFrame, SInt64 endFrame)
{
	CAAudioFile &file = player.GetFile();
	if (startFrame < 0) {
		startFrame = 0;
	}
	if (endFrame <= 0) {
		SInt64 numFrames = SInt64(file.GetNumberFrames() * file.GetClientDataFormat().mSampleRate / file.GetFileDataFormat().mSampleRate);
		endFrame = numFrames;
	}
	player.Start();
	while (true) {
		SInt64 pos = player.GetCurrentFrame();
		//file.Tell();
		if (pos >= endFrame)
			break;
		CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.25, false);
	}
	player.Stop();
}

void	MissingArgument()
{
	fprintf(stderr, "Missing argument\n");
	usage();
}

#if !TARGET_OS_WIN32
static AudioDeviceID	AudioDeviceByName(const char *findName)
{
	OSStatus err;
	UInt32 size;
	AudioDeviceID *devices, result = kAudioDeviceUnknown;
	
	err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &size, NULL);
	if (err) return kAudioDeviceUnknown;

	int n = size / sizeof(AudioDeviceID);
	devices = new AudioDeviceID[n];
	err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &size, devices);
	if (!err) {
		for (int i = 0; i < n; ++i) {
			char name[512];
			size = sizeof(name);
			err = AudioDeviceGetProperty(devices[i], 0, 0, kAudioDevicePropertyDeviceName, &size, name);
			if (!err) {
				if (!strcmp(name, findName)) {
					result = devices[i];
					break;
				}
			}
		}
	}
	if(devices)
		delete devices;
		
	return result;
}
#endif

int main(int argc, const char *argv[])
{
	const char *fileToPlay = NULL;
	bool playallpairs = false;
	double startSeconds = -1, endSeconds = -1;
	SInt64 startFrame = 0, endFrame = 0;
	int trackNum = 0;
	bool verbose = false;
	
	#if TARGET_OS_WIN32
		InitializeQTML(0L);
	#else
		AudioDeviceID theDevice = kAudioDeviceUnknown;
	#endif
	
	for (int i = 1; i < argc; ++i) {
		const char *arg = argv[i];
		if (arg[0] != '-') {
			if (fileToPlay != NULL) {
				fprintf(stderr, "may only specify one file to play\n");
				usage();
			}
			fileToPlay = arg;
		} else {
			arg += 1;
			if (arg[0] == 'a' || !strcmp(arg, "-all"))
				playallpairs = true;
			else if (arg[0] == 'f' || !strcmp(arg, "-frame")) {
				if (++i == argc)
					MissingArgument();
				arg = argv[i];
				sscanf(arg, "%qd", &startFrame);
			} else if (arg[0] == 't' || !strcmp(arg, "-track")) {
				if (++i == argc)
					MissingArgument();
				arg = argv[i];
				sscanf(arg, "%d", &trackNum);
			} else if (arg[0] == 'v' || !strcmp(arg, "-verbose")) {
				verbose = true;
			} else if (arg[0] == 's' || !strcmp(arg, "-seconds")) {
				if (++i == argc)
					MissingArgument();
				arg = argv[i];
				sscanf(arg, "%lf", &startSeconds);
				if (i + 1 < argc && argv[i+1][0] != '-') {
					arg = argv[++i];
					sscanf(arg, "%lf", &endSeconds);
				}
			#if !TARGET_OS_WIN32
			} else if (arg[0] == 'd' || !strcmp(arg, "-device")) {
				if (++i == argc)
					MissingArgument();
				arg = argv[i];
				theDevice = AudioDeviceByName(arg);
				if (theDevice == kAudioDeviceUnknown)
					fprintf(stderr, "warning: device '%s' not found\n", arg);
			#endif
			} else if (arg[0] == 'h' || !strcmp(arg, "-help")) {
				usage();
			} else {
				fprintf(stderr, "unknown argument: %s\n\n", arg - 1);
				usage();
			}
		}
	}

	if (fileToPlay == NULL)
		usage();

	try {
		const int kNumberBuffers = 3;
		const unsigned kBufferSize = 0x1000;
		FSRef playFSRef;
		XThrowIfError(FSPathMakeRef((UInt8 *)fileToPlay, &playFSRef, NULL), "Input file not found");
#if !TARGET_OS_WIN32
		CAChannelMappingPlayer player(kNumberBuffers, kBufferSize, theDevice);
#else
		CAChannelMappingPlayer player(kNumberBuffers, kBufferSize);
#endif

		player.SetFile(playFSRef, playallpairs, trackNum);
		CAAudioFile &file = player.GetFile();
		if (verbose)
			file.GetFileDataFormat().PrintFormat(stdout, "", "Data format:   ");
		CAChannelMapper *mapper = player.GetMapper();
		
		if (playallpairs && mapper == NULL)
			playallpairs = false;
		
		if (startSeconds >= 0) {
			Float64 sr = file.GetClientDataFormat().mSampleRate;
			startFrame = SInt64(startSeconds * sr);
			if (endSeconds > startSeconds)
				endFrame = SInt64(endSeconds * sr);
			printf("secs: %.1f-%.1f  frames: %qd-%qd\n", startSeconds, endSeconds, startFrame, endFrame);
		}
		
		if (playallpairs) {
			int nChannelsInFile = file.GetFileDataFormat().NumberChannels();
			for (int channel = 0; channel < nChannelsInFile && playallpairs; channel += 2) {
				printf("channels %d-%d of %d\n", channel, channel+1, nChannelsInFile);
				mapper->ResetMixer();
				mapper->ConnectChannelToChannel(channel, 0);
				mapper->ConnectChannelToChannel(channel+1, 1);
				player.SetCurrentPosition(0.);
				Play(player, startFrame, endFrame);
			}
		} else {
			file.Seek(startFrame);
			Play(player, startFrame, endFrame);
		}
	}
	catch (CAXException &e) {
		char buf[256];
		fprintf(stderr, "Error: %s (%s)\n", e.mOperation, CAXException::FormatError(buf, e.mError));
		return 1;
	}
	catch (...) {
		fprintf(stderr, "An unknown error occurred\n");
		return 1;
	}
	return 0;
}
