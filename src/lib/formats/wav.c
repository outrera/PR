/*  Princed V3 - Prince of Persia Level Editor for PC Version
    Copyright (C) 2003 Princed Development Team

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    The authors of this program may be contacted at http://forum.princed.org
*/

/*
wav.c: Princed Resources : WAV files support
�����
 Copyright 2003 Princed Development Team
  Created: 24 Aug 2003

  Author: Enrique Calot <ecalot.cod@princed.org>
  Version: 1.01 (2003-Oct-23)

 Note:
  DO NOT remove this copyright notice
*/

/* Includes */
#include "dat.h"
#include "disk.h"
#include "wav.h"
#include <string.h>
#include <stdlib.h>

#define ROUND_UP_TO_EVEN(x) (((x) + 1) & ~1)

int writeWav(const char* file, tWave* snd, int optionflag, const char* backupExtension) {
	FILE*         target;
	int ok;
	unsigned char wav[]=WAVE_HEADER;
	int sndsize = snd->samples.size;
	int sndsize_pad = ROUND_UP_TO_EVEN(sndsize);
	int riffsize = sndsize_pad+36;
	int samplerate = snd->samplerate;

	ok=writeOpen(file,&target,optionflag);

	wav[4]=(unsigned char)(riffsize&0xFF);
	wav[5]=(unsigned char)((riffsize>>8)&0xFF);
	wav[6]=(unsigned char)((riffsize>>16)&0xFF);
	wav[7]=(unsigned char)((riffsize>>24)&0xFF);

	wav[0x18] = samplerate & 0xFF;
	wav[0x19] = (samplerate >> 8) & 0xFF;
	wav[0x1A] = (samplerate >> 16) & 0xFF;
	wav[0x1B] = (samplerate >> 24) & 0xFF;
	wav[0x1C] = samplerate & 0xFF;
	wav[0x1D] = (samplerate >> 8) & 0xFF;
	wav[0x1E] = (samplerate >> 16) & 0xFF;
	wav[0x1F] = (samplerate >> 24) & 0xFF;

	wav[40]=(unsigned char)((sndsize)&0xFF);
	wav[41]=(unsigned char)(((sndsize)>>8)&0xFF);
	wav[42]=(unsigned char)(((sndsize)>>16)&0xFF);
	wav[43]=(unsigned char)(((sndsize)>>24)&0xFF);

	ok=ok&&fwrite(wav,sizeof(wav),1,target);
	ok=ok&&fwrite(snd->samples.data,snd->samples.size,1,target);
	ok=ok&&fwrite("",sndsize_pad-sndsize,1,target);
	ok=ok&&(!writeCloseOk(target,optionflag,backupExtension));

	return ok?PR_RESULT_SUCCESS:PR_RESULT_ERR_FILE_NOT_WRITE_ACCESS;
}

int readWav(const char* file, tBinary* snd, int *pchannels, long *psamplerate, long *pbps) {
	FILE* fd;
	int ok;
	char magic[4];
	long int ChunkSize=0; /* longs must be initialized to avoid trash in 64 bits architectures */
	long int SubChunk1Size=0;
	short int AudioFormat;
	short int NumChannels;
	long int SampleRate=0;
	long int ByteRate=0;
	short int BlockAlign;
	short int BitsPerSample;
	long int SubChunk2Size=0;
	
	fd=fopen(file,"rb");
	if (!fd) return PR_RESULT_ERR_FILE_NOT_READ_ACCESS;

	/* Read headers */
	ok=fread(magic,4,1,fd);
	ok=ok&&!strncmp(magic,"RIFF",4);
	ok=ok&&freadlong(&ChunkSize,fd);
	ok=ok&&fread(magic,4,1,fd);
	ok=ok&&!strncmp(magic,"WAVE",4);
	ok=ok&&fread(magic,4,1,fd);
	ok=ok&&!strncmp(magic,"fmt ",4);
	ok=ok&&freadlong(&SubChunk1Size,fd);
	ok=ok&&freadshort(&AudioFormat,fd);
	ok=ok&&freadshort(&NumChannels,fd);
	ok=ok&&freadlong(&SampleRate,fd);
	ok=ok&&freadlong(&ByteRate,fd);
	ok=ok&&freadshort(&BlockAlign,fd);
	ok=ok&&freadshort(&BitsPerSample,fd);
	ok=ok&&fread(magic,4,1,fd);
	ok=ok&&!strncmp(magic,"data",4);
	ok=ok&&freadlong(&SubChunk2Size,fd);
	
	/* Validate input vars */	
  ok=ok&& (AudioFormat   == 1 ); /* PCM */
  ok=ok&& (BlockAlign    == NumChannels * BitsPerSample/8 );
/*	ok=ok&& ((int)ByteRate      == (int)(SampleRate * NumChannels * BitsPerSample/8) ); * why int? because I can't compare it with long, never tried in 32 bits */
	ok=ok&& ((int)ChunkSize     == (int)(4 + (8 + SubChunk1Size) + (8 + ROUND_UP_TO_EVEN(SubChunk2Size)) ));
  ok=ok&& ((int)SubChunk1Size == (int)16 ); /* PCM chunk */
/*	ok=ok&& (SubChunk2Size == NumSamples * NumChannels * BitsPerSample/8 );*/
	
	/* Read data*/
	if (ok) {
		snd->size=SubChunk2Size;
		snd->data=malloc(SubChunk2Size);
		ok=fread(snd->data,SubChunk2Size,1,fd);
		fclose(fd);
	} else {
		fclose(fd);
		return PR_RESULT_BAD_FILE_FORMAT;
	}
	/* TODO: check eof */
	
	/*
	unsigned char wav[]=WAVE_HEADER;
	int i=sizeof(wav);
	unsigned char* posAux=res->content.data;

	if (res->content.size<=i) return 0; * false *
	res->content.size-=(--i);
	while ((i==4||i==5||i==6||i==7||i==40||i==41||i==42||i==43||((res->content.data)[i]==wav[i]))&&(i--));
	(res->content.data)[sizeof(wav)-1]=1; * First character must be a 0x01 (wav type in DAT) *
	res->content.data+=sizeof(wav)-1;
	if (i==-1) mWriteFileInDatFile(res);
	res->content.data=posAux;
	return 1; * true */

	if (!ok) {
		free(snd->data);
		return PR_RESULT_BAD_FILE_FORMAT;
	}
	
	*pchannels    = NumChannels;
	*psamplerate  = SampleRate;
	*pbps         = BitsPerSample;
	
	return PR_RESULT_SUCCESS;			
}

