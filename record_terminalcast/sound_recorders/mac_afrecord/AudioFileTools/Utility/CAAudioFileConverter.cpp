/*	Copyright � 2007 Apple Inc. All Rights Reserved.
	
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
#include "CAAudioFileConverter.h"
#include "CAChannelLayouts.h"
#include <sys/stat.h>
#include <algorithm>
#include "CAXException.h"
#include "CAFilePathUtils.h"
#if !defined(__COREAUDIO_USE_FLAT_INCLUDES__)
	#include <AudioUnit/AudioCodec.h>
#else
	#include <AudioCodec.h>
#endif

CAAudioFileConverter::ConversionParameters::ConversionParameters() :
	flags(0)
{
	memset(&input, 0, sizeof(input));
	memset(&output, 0, sizeof(output));
	output.channels = -1;
	output.bitRate = -1;
	output.codecQuality = -1;
	output.srcQuality = -1;
	output.srcComplexity = 0;
	output.strategy = -1;
	output.primeMethod = -1;
	output.creationFlags = 0;
}

CAAudioFileConverter::CAAudioFileConverter() :
	mReadBuffer(NULL),
	mReadPtrs(NULL)
{
	mOutName[0] = '\0';
}

CAAudioFileConverter::~CAAudioFileConverter()
{
}

void	CAAudioFileConverter::GenerateOutputFileName(const char *inputFilePath, 
						const CAStreamBasicDescription &inputFormat,
						const CAStreamBasicDescription &outputFormat, OSType outputFileType, 
						char *outName)
{
	struct stat sb;
	char inputDir[256];
	char inputBasename[256];

	strcpy(inputDir, dirname((char*)inputFilePath));
	const char *infname = basename((char*)inputFilePath);
	const char *inext = strrchr(infname, '.');
	if (inext == NULL) strcpy(inputBasename, infname);
	else {
		int n;
		memcpy(inputBasename, infname, n = inext - infname);
		inputBasename[n] = '\0';
	}
	
	CFArrayRef exts;
	UInt32 propSize = sizeof(exts);
	XThrowIfError(AudioFileGetGlobalInfo(kAudioFileGlobalInfo_ExtensionsForType,
		sizeof(OSType), &outputFileType, &propSize, &exts), "generate output file name");
	char outputExt[32];
	CFStringRef cfext = (CFStringRef)CFArrayGetValueAtIndex(exts, 0);
	CFStringGetCString(cfext, outputExt, sizeof(outputExt), kCFStringEncodingUTF8);
	CFRelease(exts);
	
	// 1. olddir + oldname + newext
	sprintf(outName, "%s/%s.%s", inputDir, inputBasename, outputExt);
#if TARGET_OS_MAC	
	if (lstat(outName, &sb)) return;
#else
	if (stat(outName, &sb)) return;
#endif

	if (outputFormat.IsPCM()) {
		// If sample rate changed:
		//	2. olddir + oldname + "-SR" + newext
		if (inputFormat.mSampleRate != outputFormat.mSampleRate && outputFormat.mSampleRate != 0.) {
			sprintf(outName, "%s/%s-%.0fk.%s", inputDir, inputBasename, outputFormat.mSampleRate/1000., outputExt);
#if TARGET_OS_MAC	
			if (lstat(outName, &sb)) return;
#else
			if (stat(outName, &sb)) return;
#endif
		}
		// If bit depth changed:
		//	3. olddir + oldname + "-bit" + newext
		if (inputFormat.mBitsPerChannel != outputFormat.mBitsPerChannel) {
			sprintf(outName, "%s/%s-%ldbit.%s", inputDir, inputBasename, outputFormat.mBitsPerChannel, outputExt);
#if TARGET_OS_MAC	
			if (lstat(outName, &sb)) return;
#else
			if (stat(outName, &sb)) return;
#endif
		}
	}
	
	// maybe more with channels/layouts? $$$
	
	// now just append digits
	for (int i = 1; ; ++i) {
		sprintf(outName, "%s/%s-%d.%s", inputDir, inputBasename, i, outputExt);
#if TARGET_OS_MAC	
		if (lstat(outName, &sb)) return;
#else
		if (stat(outName, &sb)) return;
#endif
	}
}

void	CAAudioFileConverter::PrintFormats(const CAAudioChannelLayout *origSrcFileLayout)
{
	const CAAudioChannelLayout &srcFileLayout = mSrcFile.GetFileChannelLayout();
	const CAAudioChannelLayout &destFileLayout = mDestFile.GetFileChannelLayout();
	
	// see where we've gotten
	if (mParams.flags & kOpt_Verbose) {
		printf("Formats:\n");
		mSrcFile.GetFileDataFormat().PrintFormat(stdout, "  ", "Input file   ");
		if (srcFileLayout.IsValid()) {
			printf("                 %s", 
				CAChannelLayouts::ConstantToString(srcFileLayout.Tag()));
			if (srcFileLayout.IsValid() && origSrcFileLayout != NULL &&
			srcFileLayout != *origSrcFileLayout)
				printf(" -- overriding layout %s in file", 
					CAChannelLayouts::ConstantToString(origSrcFileLayout->Tag()));
			printf("\n");
		}
		mDestFile.GetFileDataFormat().PrintFormat(stdout, "  ", "Output file  ");
		if (destFileLayout.IsValid())
			printf("                 %s\n", 
				CAChannelLayouts::ConstantToString(destFileLayout.Tag()));
		if (mSrcFile.HasConverter()) {
			mSrcFile.GetClientDataFormat().PrintFormat(stdout, "  ", "Input client ");
			CAShow(mSrcFile.GetConverter());
		}
		if (mDestFile.HasConverter()) {
			mDestFile.GetClientDataFormat().PrintFormat(stdout, "  ", "Output client");
			CAShow(mDestFile.GetConverter());
		}
	}
}

void	CAAudioFileConverter::OpenInputFile()
{
	AudioFileID fileid = mParams.input.audioFileID;
	if (mParams.input.trackIndex != 0) {
		if (fileid == 0) {
			CACFURL url = CFURLCreateFromFileSystemRepresentation(NULL, (const Byte *)mParams.input.filePath, strlen(mParams.input.filePath), false);
			XThrowIfError(AudioFileOpenURL(url.GetCFObject(), fsRdPerm, 0, &fileid), "Couldn't open input file");
		}
		XThrowIfError(AudioFileSetProperty(fileid, 'uatk' /*kAudioFilePropertyUseAudioTrack*/, sizeof(mParams.input.trackIndex), &mParams.input.trackIndex), "Couldn't set input file's track index");
	}

	if (fileid)
		mSrcFile.WrapAudioFileID(fileid, false);
	else
		mSrcFile.Open(mParams.input.filePath);


}

void	CAAudioFileConverter::OpenOutputFile(const CAStreamBasicDescription &srcFormat, const CAStreamBasicDescription &destFormat, CAAudioChannelLayout &destFileLayout)
{
	const ConversionParameters &params = mParams;
	
	// output file
	if (params.output.filePath == NULL) {
		GenerateOutputFileName(params.input.filePath, srcFormat,
					destFormat, params.output.fileType, mOutName);
	} else
		strcpy(mOutName, params.output.filePath);
	
	// deal with pre-existing output file
	struct stat st;
	if (stat(mOutName, &st) == 0) {
		XThrowIf(!(params.flags & kOpt_OverwriteOutputFile), 1, "overwrite output file");
		// not allowed to overwrite
		// output file exists - delete it
		XThrowIfError(unlink(mOutName), "delete output file");
	}
	
	// create the output file
	mDestFile.Create(mOutName, params.output.fileType, destFormat, &destFileLayout.Layout(), params.output.creationFlags);
}


void	CAAudioFileConverter::ConvertFile(const ConversionParameters &_params)
{
	CAStreamBasicDescription destFormat;
	CAAudioChannelLayout origSrcFileLayout, srcFileLayout, destFileLayout;
	bool openedSourceFile = false, createdOutputFile = false;
	
	mParams = _params;
	mReadBuffer = NULL;
	mReadPtrs = NULL;
	PrepareConversion();

	try {
		if (TaggedDecodingFromCAF())
			ReadCAFInfo();
		OpenInputFile();
		openedSourceFile = true;
		
		// get input file's format and channel layout
		const CAStreamBasicDescription &srcFormat = mSrcFile.GetFileDataFormat();
		srcFileLayout = mSrcFile.GetFileChannelLayout();

		if (mParams.flags & kOpt_Verbose) {
			printf("Input file: %s, %qd frames", mParams.input.filePath ? basename((char*)mParams.input.filePath) : "?", 
				mSrcFile.GetNumberFrames());
			if (srcFileLayout.IsValid()) {
				printf(", %s", CAChannelLayouts::ConstantToString(srcFileLayout.Tag()));
			}
			printf("\n");
		}
		mSrcFormat = srcFormat;
		
		// prepare output file's format
		destFormat = mParams.output.dataFormat;

		bool encoding = !destFormat.IsPCM();
		bool decoding = !srcFormat.IsPCM();
		
		if (!encoding && destFormat.mSampleRate == 0.)
			// on encode, it's OK to have a 0 sample rate; ExtAudioFile will get the SR from the converter and set it on the file.
			// on decode or PCM->PCM, a sample rate of 0 is interpreted as using the source sample rate
			destFormat.mSampleRate = srcFormat.mSampleRate;
		

		origSrcFileLayout = srcFileLayout;
		if (mParams.input.channelLayoutTag != 0) {
			XThrowIf(AudioChannelLayoutTag_GetNumberOfChannels(mParams.input.channelLayoutTag)
				!= srcFormat.mChannelsPerFrame, -1, "input channel layout has wrong number of channels for file");
			srcFileLayout = CAAudioChannelLayout(mParams.input.channelLayoutTag);
			mSrcFile.SetFileChannelLayout(srcFileLayout);
		}
		
		// destination channel layout
		int outChannels = mParams.output.channels;
		bool destLayoutDefaulted = false;
		if (mParams.output.channelLayoutTag != 0) {
			// use the one specified by caller, if any
			destFileLayout = CAAudioChannelLayout(mParams.output.channelLayoutTag);
		} else if (srcFileLayout.IsValid()) {
			// otherwise, assume the same as the source, if any
			destFileLayout = srcFileLayout;
			destLayoutDefaulted = true;
		}
		if (destFileLayout.IsValid()) {
			// the output channel layout specifies the number of output channels
			if (outChannels != -1)
				XThrowIf((unsigned)outChannels != destFileLayout.NumberChannels(), -1,
					"output channel layout has wrong number of channels");
			else
				outChannels = destFileLayout.NumberChannels();
		}

		if (!(mParams.flags & kOpt_NoSanitizeOutputFormat)) {
			// adjust the output format's channels; output.channels overrides the channels
			if (outChannels == -1)
				outChannels = srcFormat.mChannelsPerFrame;
			if (outChannels > 0) {
				destFormat.mChannelsPerFrame = outChannels;
				destFormat.mBytesPerPacket *= outChannels;
				destFormat.mBytesPerFrame *= outChannels;
			}
		
		}

		CAStreamBasicDescription srcClientFormat, destClientFormat;
		bool compressed2compressed = false;
		if ((encoding || decoding) && srcFormat == destFormat) {
			// identical compressed formats; should do packet-by-packet copying
			destFormat = srcFormat;	// the above equality test includes wildcards; we really want an exact copy
			compressed2compressed = true;
			encoding = decoding = false;
			srcClientFormat = srcFormat;
		}
		
		OpenOutputFile(srcFormat, destFormat, destFileLayout);
		createdOutputFile = true;
		mDestFormat = destFormat;
		
		if (!compressed2compressed) {
			// set up client formats
			{
				CAAudioChannelLayout srcClientLayout, destClientLayout;
				
				if (encoding) {
					if (decoding) {
	//					XThrowIf(encoding && decoding, -1, "transcoding not currently supported");
						if (srcFormat != destFormat) {
							// transcoding
							if (srcFormat.mChannelsPerFrame > 2 || destFormat.mChannelsPerFrame > 2)
								CAXException::Warning("Transcoding multichannel audio may not handle channel layouts correctly", 0);
							srcClientFormat.SetCanonical(std::min(srcFormat.mChannelsPerFrame, destFormat.mChannelsPerFrame), true);
							srcClientFormat.mSampleRate = std::max(srcFormat.mSampleRate, destFormat.mSampleRate);
							mSrcFile.SetClientFormat(srcClientFormat, NULL);
							
							destClientFormat = srcClientFormat;
						}
					} else {
						// encoding
						srcClientFormat = srcFormat;

						// if we're removing channels, do it on the source file
						// (solves problems with mono-only codecs like AMR and is more efficient)
						if (srcFormat.mChannelsPerFrame > destFormat.mChannelsPerFrame) {
							srcClientFormat.ChangeNumberChannels(destFormat.mChannelsPerFrame, true);
							mSrcFile.SetClientFormat(srcClientFormat, NULL);
						}
						destClientFormat = srcClientFormat;
					}
					// by here, destClientFormat will have a valid sample rate
					destClientLayout = srcFileLayout.IsValid() ? srcFileLayout : destFileLayout;

					mDestFile.SetClientFormat(destClientFormat);
					if (destLayoutDefaulted) {
						try {
							mDestFile.SetClientChannelLayout(destClientLayout);
						}
						catch (CAXException &e) { }	// ignore failure here
					} else
						mDestFile.SetClientChannelLayout(destClientLayout);	// must succeed
				} else {
					// decoding or PCM->PCM
					if (destFormat.mSampleRate == 0.)
						destFormat.mSampleRate = srcFormat.mSampleRate;
			
					destClientFormat = destFormat;
					srcClientFormat = destFormat;
					srcClientLayout = destFileLayout;
					
					mSrcFile.SetClientFormat(srcClientFormat, &srcClientLayout);
				}
			}
			
			XThrowIf(srcClientFormat.mBytesPerPacket == 0, -1, "source client format not PCM"); 
			XThrowIf(destClientFormat.mBytesPerPacket == 0, -1, "dest client format not PCM"); 		
			if (encoding) {
				// set the codec quality
				if (mParams.output.codecQuality != -1) {
					if (mParams.flags & kOpt_Verbose)
						printf("codec quality = %ld\n", mParams.output.codecQuality);
					mDestFile.SetConverterProperty(kAudioConverterCodecQuality, sizeof(UInt32), &mParams.output.codecQuality);
				}

				// set the bitrate strategy -- called bitrate format in the codecs since it had already shipped
				if (mParams.output.strategy != -1) {
					if (mParams.flags & kOpt_Verbose)
						printf("strategy = %ld\n", mParams.output.strategy);
					mDestFile.SetConverterProperty(kAudioCodecBitRateFormat, sizeof(UInt32), &mParams.output.strategy);
				}
				
				// set any user defined properties
				UInt32 i = 0;
				while (mParams.output.userProperty[i].propertyID != 0 && i < kMaxUserProps)
				{
					if (mParams.flags & kOpt_Verbose)
						printf("user property '%4.4s' = %ld\n", (char *)(&mParams.output.userProperty[i].propertyID), mParams.output.userProperty[i].propertyValue);
					mDestFile.SetConverterProperty(mParams.output.userProperty[i].propertyID, sizeof(SInt32), &mParams.output.userProperty[i].propertyValue);
					++i;
				}

				// set the bitrate
				if (mParams.output.bitRate != -1) {
					if (mParams.flags & kOpt_Verbose)
						printf("bitrate = %ld\n", mParams.output.bitRate);
					mDestFile.SetConverterProperty(kAudioConverterEncodeBitRate, sizeof(UInt32), &mParams.output.bitRate);
				}
			}
			if (srcFormat.mSampleRate != 0. && destFormat.mSampleRate != 0. && srcFormat.mSampleRate != destFormat.mSampleRate) 
			{
				// set the SRC quality
				if (mParams.output.srcQuality != -1) {
					if (mParams.flags & kOpt_Verbose)
						printf("SRC quality = %ld\n", mParams.output.srcQuality);
					if (encoding)
						mDestFile.SetConverterProperty(kAudioConverterSampleRateConverterQuality, sizeof(UInt32), &mParams.output.srcQuality);
					else
						mSrcFile.SetConverterProperty(kAudioConverterSampleRateConverterQuality, sizeof(UInt32), &mParams.output.srcQuality);
				}

				// set the SRC complexity
				if (mParams.output.srcComplexity) {
					if (mParams.flags & kOpt_Verbose)
						printf("SRC complexity = '%4.4s'\n", (char *)(&mParams.output.srcComplexity));
					if (encoding)
						mDestFile.SetConverterProperty('srca'/*kAudioConverterSampleRateConverterComplexity*/, sizeof(UInt32), &mParams.output.srcComplexity, true);
					else
						mSrcFile.SetConverterProperty('srca'/*kAudioConverterSampleRateConverterComplexity*/, sizeof(UInt32), &mParams.output.srcComplexity, true);
				}
			}
			if (decoding) {
				if (mParams.output.primeMethod != -1)
					mSrcFile.SetConverterProperty(kAudioConverterPrimeMethod, sizeof(UInt32), &mParams.output.primeMethod);
				// set any user defined properties
				UInt32 i = 0;
				while (mParams.output.userProperty[i].propertyID != 0 && i < kMaxUserProps)
				{
					if (mParams.flags & kOpt_Verbose)
						printf("user property '%4.4s' = %ld\n", (char *)(&mParams.output.userProperty[i].propertyID), mParams.output.userProperty[i].propertyValue);
					mSrcFile.SetConverterProperty(mParams.output.userProperty[i].propertyID, sizeof(SInt32), &mParams.output.userProperty[i].propertyValue);
					++i;
				}
			}
			if (mParams.output.creationFlags & kAudioFileFlags_DontPageAlignAudioData) {
				if (mParams.flags & kOpt_Verbose)
					printf("no filler chunks\n");
			}
		}
		PrintFormats(&origSrcFileLayout);

		// prepare I/O buffers
		UInt32 bytesToRead = 0x10000;
		UInt32 framesToRead = bytesToRead;	// OK, ReadPackets will limit as appropriate
		ComputeReadSize(srcFormat, destFormat, bytesToRead, framesToRead);

		mReadBuffer = CABufferList::New("readbuf", srcClientFormat);
		mReadBuffer->AllocateBuffers(bytesToRead);
		mReadPtrs = CABufferList::New("readptrs", srcClientFormat);
		
		BeginConversion();
		
		while (true) {
			UInt32 nFrames = framesToRead;
			mReadPtrs->SetFrom(mReadBuffer);
			AudioBufferList *readbuf = &mReadPtrs->GetModifiableBufferList();
			
			mSrcFile.Read(nFrames, readbuf);
			//printf("read %ld of %ld frames\n", nFrames, framesToRead);
			if (nFrames == 0)
				break;

			mDestFile.Write(nFrames, readbuf);
			if (ShouldTerminateConversion())
				break;
		}
		
#if 0
		if (decoding) {
			// fix up the destination file's length if necessary and possible
			SInt64 nframes = mSrcFile.GetNumberFrames();
			if (nframes != 0) {
				// only shorten, don't try to lengthen
#warning shouldn't be doing this here
				nframes = SInt64(ceil(nframes * destFormat.mSampleRate / srcFormat.mSampleRate));
				if (nframes < mDestFile.GetNumberFrames()) {
					mDestFile.SetNumberFrames(nframes);
				}
			}
		}
#endif
		EndConversion();
	}
	catch (...) {
		delete mReadBuffer;
		delete mReadPtrs;
		try { mSrcFile.Close(); } catch (...) { }
		try { mDestFile.Close(); } catch (...) { }
		if (createdOutputFile)
			unlink(mOutName);
		throw;
	}
	delete mReadBuffer;
	delete mReadPtrs;
	mSrcFile.Close();
	mDestFile.Close();
	if (TaggedEncodingToCAF())
		WriteCAFInfo();
	
	if (mParams.flags & kOpt_Verbose) {
		// must close to flush encoder; GetNumberFrames() not necessarily valid until afterwards but then
		// the file is closed
		CAExtAudioFile temp;
		temp.Open(mOutName);
		printf("Output file: %s, %qd frames\n", basename(mOutName), temp.GetNumberFrames());
	}
}

#define kMaxFilename 64
struct CAFSourceInfo {
	// our private user data chunk -- careful about compiler laying this out!
	// big endian
	char	asbd[40];
	UInt32	filetype;
	char	filename[kMaxFilename];
};

static void ASBD_NtoB(const AudioStreamBasicDescription *infmt, AudioStreamBasicDescription *outfmt)
{
	*(UInt64 *)&outfmt->mSampleRate = EndianU64_NtoB(*(UInt64 *)&infmt->mSampleRate);
	outfmt->mFormatID = EndianU32_NtoB(infmt->mFormatID);
	outfmt->mFormatFlags = EndianU32_NtoB(infmt->mFormatFlags);
	outfmt->mBytesPerPacket = EndianU32_NtoB(infmt->mBytesPerPacket);
	outfmt->mFramesPerPacket = EndianU32_NtoB(infmt->mFramesPerPacket);
	outfmt->mBytesPerFrame = EndianU32_NtoB(infmt->mBytesPerFrame);
	outfmt->mChannelsPerFrame = EndianU32_NtoB(infmt->mChannelsPerFrame);
	outfmt->mBitsPerChannel = EndianU32_NtoB(infmt->mBitsPerChannel);
}

static void ASBD_BtoN(const AudioStreamBasicDescription *infmt, AudioStreamBasicDescription *outfmt)
{
	*(UInt64 *)&outfmt->mSampleRate = EndianU64_BtoN(*(UInt64 *)&infmt->mSampleRate);
	outfmt->mFormatID = EndianU32_BtoN(infmt->mFormatID);
	outfmt->mFormatFlags = EndianU32_BtoN(infmt->mFormatFlags);
	outfmt->mBytesPerPacket = EndianU32_BtoN(infmt->mBytesPerPacket);
	outfmt->mFramesPerPacket = EndianU32_BtoN(infmt->mFramesPerPacket);
	outfmt->mBytesPerFrame = EndianU32_BtoN(infmt->mBytesPerFrame);
	outfmt->mChannelsPerFrame = EndianU32_BtoN(infmt->mChannelsPerFrame);
	outfmt->mBitsPerChannel = EndianU32_BtoN(infmt->mBitsPerChannel);
}

void	CAAudioFileConverter::WriteCAFInfo()
{
	AudioFileID afid = 0;
	CAFSourceInfo info;
	UInt32 size;
	
	try {
		CACFURL url = CFURLCreateFromFileSystemRepresentation(NULL, (UInt8 *)mParams.input.filePath, strlen(mParams.input.filePath), false);
		XThrowIf(url.GetCFObject() == NULL, -1, "couldn't locate input file");
		XThrowIfError(AudioFileOpenURL(url.GetCFObject(), fsRdPerm, 0, &afid), "couldn't open input file");
		size = sizeof(AudioFileTypeID);
		XThrowIfError(AudioFileGetProperty(afid, kAudioFilePropertyFileFormat, &size, &info.filetype), "couldn't get input file's format");
		AudioFileClose(afid);
		afid = 0;
		
		url = CFURLCreateFromFileSystemRepresentation(NULL, (UInt8 *)mOutName, strlen(mOutName), false);
		XThrowIf(url.GetCFObject() == NULL, -1, "couldn't locate output file");
		XThrowIfError(AudioFileOpenURL(url.GetCFObject(), fsRdWrPerm, 0, &afid), "couldn't open output file");
		const char *srcFilename = strrchr(mParams.input.filePath, '/');
		if (srcFilename++ == NULL) srcFilename = mParams.input.filePath;
		ASBD_NtoB(&mSrcFormat, (AudioStreamBasicDescription *)info.asbd);
		int namelen = std::min(kMaxFilename-1, (int)strlen(srcFilename));
		memcpy(info.filename, srcFilename, namelen);
		info.filename[namelen++] = 0;
		info.filetype = EndianU32_NtoB(info.filetype);
		
		XThrowIfError(AudioFileSetUserData(afid, 'srcI', 0, offsetof(CAFSourceInfo, filename) + namelen, &info), "couldn't set CAF file's source info chunk");
		AudioFileClose(afid);
	}
	catch (...) {
		if (afid)
			AudioFileClose(afid);
		throw;
	}
}

void	CAAudioFileConverter::ReadCAFInfo()
{
	AudioFileID afid = 0;
	CAFSourceInfo info;
	UInt32 size;
	OSStatus err;
	
	try {
		CACFURL url = CFURLCreateFromFileSystemRepresentation(NULL, (UInt8 *)mParams.input.filePath, strlen(mParams.input.filePath), false);
		XThrowIf(!url.IsValid(), -1, "couldn't locate input file");
		XThrowIfError(AudioFileOpenURL(url.GetCFObject(), fsRdPerm, 0, &afid), "couldn't open input file");
		size = sizeof(AudioFileTypeID);
		XThrowIfError(AudioFileGetProperty(afid, kAudioFilePropertyFileFormat, &size, &info.filetype), "couldn't get input file's format");
		if (info.filetype == kAudioFileCAFType) {
			size = sizeof(info);
			err = AudioFileGetUserData(afid, 'srcI', 0, &size, &info);
			if (!err) {
				// restore the following from the original file info:
				//	filetype
				//	data format
				//	filename
				AudioStreamBasicDescription destfmt;
				ASBD_BtoN((AudioStreamBasicDescription *)info.asbd, &destfmt);
				mParams.output.dataFormat = destfmt;
				mParams.output.fileType = EndianU32_BtoN(info.filetype);
				if (mParams.output.filePath == NULL) {
					int len = strlen(mParams.input.filePath) + strlen(info.filename) + 2;
					char *newname = (char *)malloc(len);	// $$$ leaked
					const char *dir = dirname((char*)mParams.input.filePath);
					if (dir && (dir[0] !='.' && dir[1] != '/'))
						sprintf(newname, "%s/%s", dir, info.filename);
					else
						strcpy(newname, info.filename);
					mParams.output.filePath = newname;
					mParams.flags = (mParams.flags & ~kOpt_OverwriteOutputFile) | kOpt_NoSanitizeOutputFormat;
				}
			}
		}
		AudioFileClose(afid);
	}
	catch (...) {
		if (afid)
			AudioFileClose(afid);
		throw;
	}
}

