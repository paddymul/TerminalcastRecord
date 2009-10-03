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
#if !defined(__COREAUDIO_USE_FLAT_INCLUDES__)
	#include <AudioToolbox/CAFFile.h>
	#include <AudioToolbox/AudioToolbox.h>
	#include <CoreServices/CoreServices.h>
#else
	#include <CAFFile.h>
	#include <AudioToolbox.h>
	#include <CoreServices.h>
#endif

#if TARGET_OS_WIN32
	#include <QTML.h>
#endif

#include <stdio.h>
#include "CAStreamBasicDescription.h"
#include <stdlib.h>

static void	PrintMarkerList(const char *indent, const AudioFileMarker *markers, int nMarkers);
static void ParseAudioFile (AudioFileID afid, const char* fname);

static void usage()
{
	fprintf(stderr,
			"Usage:\n"
			"%s  audio_file(s)\n\n"
#if !TARGET_OS_WIN32
			, getprogname());
#else
			, "afinfo");
#endif
	exit(1);
}


int main (int argc, const char * argv[]) 
{
#if TARGET_OS_WIN32
	InitializeQTML(0L);
#endif
	if (argc < 2)
		usage();
		
	for (int i=1; i<argc; ++i)	
	{		
		CFURLRef url = CFURLCreateFromFileSystemRepresentation(NULL, (const UInt8*)argv[i], strlen(argv[i]), false);
		if (!url) {
			fprintf(stderr, "Can't create CFURL for '%s'\n", argv[i]);
			continue;
		}
		
		AudioFileID afid;
		OSStatus err = AudioFileOpenURL(url, fsRdPerm, 0, &afid);
		if (err) {
			fprintf(stderr, "AudioFileOpenURL failed for '%s'\n", argv[i]);
			goto Bail;
		}

		AudioFileTypeID filetype;
		UInt32 propertySize;
		propertySize = sizeof(AudioFileTypeID);
		err = AudioFileGetProperty(afid, kAudioFilePropertyFileFormat, &propertySize, &filetype);
		if (err) {
			fprintf(stderr, "AudioFileGetProperty kAudioFilePropertyFileFormat failed for '%s'\n", argv[i]);
			goto Bail2;
		}
		filetype = CFSwapInt32HostToBig(filetype);	// for display purposes

		printf("File:           %s\n", argv[i]);
		printf("File type ID:   %-4.4s\n", (char *)&filetype);

		UInt32 trackCount;
		propertySize = sizeof(trackCount);
		err = AudioFileGetProperty(afid, 'atct' /*kAudioFilePropertyAudioTrackCount*/, &propertySize, &trackCount);
		if (err) // file has no tracks
			trackCount = 1;

		if (trackCount > 1) {
			printf ("Num Tracks:     %d\n", (int)trackCount);
			printf ("----\n");
			for (UInt32 i = 0; i < trackCount; ++i) {
				err = AudioFileSetProperty (afid, 'uatk' /*kAudioFilePropertyUseAudioTrack*/, sizeof(i), &i);
				if (err) {
					fprintf(stderr, "AudioFileSetProperty kAudioFilePropertyUseAudioTrack failed for '%s'\n", argv[i]);
					goto Bail2;
				}
				printf ("Track:          %d\n", int(i + 1));
				ParseAudioFile (afid, argv[i]);	
			}
		} else {
			ParseAudioFile (afid, argv[i]);
		}


		Bail2:
			AudioFileClose(afid);
		Bail:
			CFRelease(url);
	}

    return 0;
}

// N.B. releases the marker names
void	PrintMarkerList(const char *indent, const AudioFileMarker *markers, int nMarkers)
{
	const AudioFileMarker *mrk = markers;
	for (int i = 0; i < nMarkers; ++i, ++mrk) {
		char name[512];
		CFStringGetCString(mrk->mName, name, sizeof(name), kCFStringEncodingUTF8);
		printf("%smarker %d, \"%s\": frame %9.0f   SMPTE time %d:%02d:%02d:%02d/%d  type %d  channel %d\n",
			indent, int(mrk->mMarkerID), name, mrk->mFramePosition, 
			mrk->mSMPTETime.mHours, mrk->mSMPTETime.mMinutes, mrk->mSMPTETime.mSeconds, mrk->mSMPTETime.mFrames, 
				int(mrk->mSMPTETime.mSubFrameSampleOffset),
			int(mrk->mType), int(mrk->mChannel));
		CFRelease(mrk->mName);
	}
}


void ParseAudioFile (AudioFileID afid, const char* fname)
{
	AudioChannelLayout *acl = 0;
	CAStreamBasicDescription asbd;
		
	UInt32 propertySize = sizeof(asbd);
	OSStatus err = AudioFileGetProperty(afid, kAudioFilePropertyDataFormat, &propertySize, &asbd);
	if (err) {
		fprintf(stderr, "AudioFileGetProperty kAudioFilePropertyDataFormat failed for '%s'\n", fname);
		goto Bail;
	}

	asbd.PrintFormat(stdout, "", "Data format:   ");

	UInt32 writable;
	err = AudioFileGetPropertyInfo(afid, kAudioFilePropertyChannelLayout, &propertySize, &writable);
	if (err) {
		//fprintf(stderr, "AudioFileGetPropertyInfo kAudioFilePropertyChannelLayout failed for '%s'\n", fname);
		printf("                no channel layout.\n");
	} else {
		
		acl = (AudioChannelLayout*)calloc(1, propertySize);
		err = AudioFileGetProperty(afid, kAudioFilePropertyChannelLayout, &propertySize, acl);
		if (err) {
			fprintf(stderr, "AudioFileGetProperty kAudioFilePropertyChannelLayout failed for '%s'\n", fname);
			goto Bail;
		}
		
		CFStringRef aclname;
		char aclstr[512];
		UInt32 specifierSize = propertySize;
		propertySize = sizeof(aclname);
		err = AudioFormatGetProperty(kAudioFormatProperty_ChannelLayoutName, specifierSize, acl, &propertySize, &aclname);
		if (err) {
			//fprintf(stderr, "AudioFileGetProperty kAudioFilePropertyChannelLayout failed for '%s'\n", fname);
			//goto Bail1;
		} else {
			CFStringGetCString(aclname, aclstr, 512, kCFStringEncodingUTF8);
		
			printf("Channel layout: %s\n", aclstr);
			CFRelease(aclname);
		}
	}
	
	Float64 estimatedDuration;
	propertySize = sizeof(estimatedDuration);
	err = AudioFileGetProperty(afid, kAudioFilePropertyEstimatedDuration, &propertySize, &estimatedDuration);
	if (err == noErr) {
		printf("estimated duration: %4.3f sec\n", estimatedDuration);
	}
			
	UInt64 dataByteCount;
	propertySize = sizeof(dataByteCount);
	err = AudioFileGetProperty(afid, kAudioFilePropertyAudioDataByteCount, &propertySize, &dataByteCount);
	if (err) {
		fprintf(stderr, "AudioFileGetProperty kAudioFilePropertyAudioDataByteCount failed for '%s'\n", fname);
	} else {
		printf("audio bytes: %llu\n", dataByteCount);
	}
	
	UInt64 dataPacketCount;
	UInt64 totalFrames;
	totalFrames = 0;
	propertySize = sizeof(dataPacketCount);
	err = AudioFileGetProperty(afid, kAudioFilePropertyAudioDataPacketCount, &propertySize, &dataPacketCount);
	if (err) {
		fprintf(stderr, "AudioFileGetProperty kAudioFilePropertyAudioDataPacketCount failed for '%s'\n", fname);
	} else {
		printf("audio packets: %llu\n", dataPacketCount);
		if (asbd.mFramesPerPacket)
			totalFrames = asbd.mFramesPerPacket * dataPacketCount;
	}
	
	AudioFilePacketTableInfo pti;
	propertySize = sizeof(pti);
	err = AudioFileGetProperty(afid, kAudioFilePropertyPacketTableInfo, &propertySize, &pti);
	if (err == noErr) {
		totalFrames = pti.mNumberValidFrames;
		printf("audio %qd valid frames + %d priming + %d remainder = %qd\n", pti.mNumberValidFrames, (unsigned)pti.mPrimingFrames, (unsigned)pti.mRemainderFrames, pti.mNumberValidFrames + pti.mPrimingFrames + pti.mRemainderFrames);
	}
	
	UInt32 bitRate;
	propertySize = sizeof(bitRate);
	err = AudioFileGetProperty(afid, 'brat', &propertySize, &bitRate);
	if (err == noErr) {
		printf("bit rate: %lu bits per second\n", bitRate);
	}

	#define kAudioFilePropertyPacketSizeUpperBound 'pkub'
	UInt32 packetSizeUpperBound;
	propertySize = sizeof(packetSizeUpperBound);
	err = AudioFileGetProperty(afid, kAudioFilePropertyPacketSizeUpperBound, &propertySize, &packetSizeUpperBound);
	if (err) {
		fprintf(stderr, "AudioFileGetProperty kAudioFilePropertyPacketSizeUpperBound failed for '%s'\n", fname);
	} else {
		printf("packet size upper bound: %lu\n", packetSizeUpperBound);
	}

	UInt32 maxPacketSize;
	propertySize = sizeof(maxPacketSize);
	err = AudioFileGetProperty(afid, kAudioFilePropertyMaximumPacketSize, &propertySize, &maxPacketSize);
	if (err) {
		fprintf(stderr, "AudioFileGetProperty kAudioFilePropertyMaximumPacketSize failed for '%s'\n", fname);
	} else {
		if (maxPacketSize != packetSizeUpperBound)
			printf("maximum packet size: %lu\n", maxPacketSize);
	}
	
	if (packetSizeUpperBound < maxPacketSize) printf("!!!!!!!!!! packetSizeUpperBound < maxPacketSize\n");
	
	UInt64 dataOffset;
	propertySize = sizeof(dataOffset);
	err = AudioFileGetProperty(afid, kAudioFilePropertyDataOffset, &propertySize, &dataOffset);
	if (err) {
		fprintf(stderr, "AudioFileGetProperty kAudioFilePropertyDataOffset failed for '%s'\n", fname);
	} else {
		printf("audio data file offset: %llu\n", dataOffset);
	}
	
	UInt32 isOptimized;
	propertySize = sizeof(isOptimized);
	err = AudioFileGetProperty(afid, kAudioFilePropertyIsOptimized, &propertySize, &isOptimized);
	if (err) {
		fprintf(stderr, "AudioFileGetProperty kAudioFilePropertyIsOptimized failed for '%s'\n", fname);
	} else {
		printf(isOptimized ? "optimized\n" : "not optimized\n");
	}

	err = AudioFileGetPropertyInfo(afid, 'flst'/*kAudioFilePropertyFormatList*/, &propertySize, &writable);
	if (err) {
		// this should never happen with an AudioToolbox that has this property defined.
		fprintf(stderr, "AudioFileGetPropertyInfo kAudioFilePropertyFormatList failed for '%s'\n", fname);
	} else {
		struct	TEMP_AudioFormatListItem {
			AudioStreamBasicDescription		mASBD;
			AudioChannelLayoutTag			mChannelLayoutTag;
		};
		if (propertySize > sizeof(TEMP_AudioFormatListItem)) 
		{
			TEMP_AudioFormatListItem*	formatList = NULL;
			AudioChannelLayout		tLayout;

			formatList = (TEMP_AudioFormatListItem*) calloc (1, propertySize);
			if (formatList)
			{
				err = AudioFileGetProperty(afid, 'flst'/*kAudioFilePropertyFormatList*/, &propertySize, formatList);
				if (err) {
					fprintf(stderr, "AudioFileGetProperty kAudioFilePropertyFormatList failed for '%s'\n", fname);
				}
				else
				{
					// the returned list may actually be smaller than GetPropertyInfo returned
					UInt32		actualListCount = propertySize/sizeof(TEMP_AudioFormatListItem);;
					//printf("FormatList: formats available: %ld\n", actualListCount);
					printf("format list:\n");
					tLayout.mChannelBitmap = 0;
					tLayout.mNumberChannelDescriptions = 0;

					for (UInt32	i = 0; i < actualListCount; i++)
					{
						//printf("FormatList index: %ld\n", i+1);
						char label[32];
						sprintf(label, "[%2ld] format:   ", i);
						// ----- print the format ASBD
						asbd = formatList[i].mASBD;
						asbd.PrintFormat(stdout, "", label);
						
						// ----- print the channel layout name
						CFStringRef layoutName;
						char aclstr[512];
						tLayout.mChannelLayoutTag = formatList[i].mChannelLayoutTag;
						
						if ((tLayout.mChannelLayoutTag & 0xFFFF0000) == 0xFFFF0000) {
							// unknown/unspecified layout
							printf("                no channel layout.\n");
						} else {
							UInt32 specifierSize = sizeof(tLayout);
							propertySize = sizeof(layoutName);
							err = AudioFormatGetProperty(kAudioFormatProperty_ChannelLayoutName, specifierSize, (void*) &tLayout, &propertySize, &layoutName);
							if (err) {
								fprintf(stderr, "Could not get the ChannelLayout name for this format list item\n");
							}
							else
							{
								CFStringGetCString(layoutName, aclstr, 512, kCFStringEncodingUTF8);
								CFRelease(layoutName);
								printf("     Channel layout: %s\n", aclstr);
							}
						}
					}
				}
			}
		}
	}

	// regions
	err = AudioFileGetPropertyInfo(afid, kAudioFilePropertyRegionList, &propertySize, NULL);
	if (err == noErr && propertySize > 0) {
		AudioFileRegionList *regionList = (AudioFileRegionList *)malloc(propertySize);
		err = AudioFileGetProperty(afid, kAudioFilePropertyRegionList, &propertySize, regionList);
		if (err == noErr) {
			printf("%d regions; SMPTE time type %d\n", int(regionList->mNumberRegions), int(regionList->mSMPTE_TimeType));
			AudioFileRegion *rgn = regionList->mRegions;
			for (UInt32 i = 0; i < regionList->mNumberRegions; ++i) {
				char name[512];
				CFStringGetCString(rgn->mName, name, sizeof(name), kCFStringEncodingUTF8);
				CFRelease(rgn->mName);
				printf("  region %d, \"%s\", flags %08X\n", int(rgn->mRegionID), name, int(rgn->mFlags));
				PrintMarkerList("    ", rgn->mMarkers, rgn->mNumberMarkers);
				rgn = NextAudioFileRegion(rgn);
			}
		}
		free(regionList);
	}

	// markers
	err = AudioFileGetPropertyInfo(afid, kAudioFilePropertyMarkerList, &propertySize, NULL);
	if (err == noErr && propertySize > 0) {
		AudioFileMarkerList *markerList = (AudioFileMarkerList *)malloc(propertySize);
		err = AudioFileGetProperty(afid, kAudioFilePropertyMarkerList, &propertySize, markerList);
		if (err == noErr) {
			printf("%d markers; SMPTE time type %d\n", int(markerList->mNumberMarkers), int(markerList->mSMPTE_TimeType));
			PrintMarkerList("  ", markerList->mMarkers, markerList->mNumberMarkers);
		}
		free(markerList);
	}
	
Bail:
	free(acl);
	
	printf("----\n");
}
