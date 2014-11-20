/*
 * id3v2parser - program to parse ID3v2.4 tags
 *
 *  Copyright (c) 2014 - Martin Rabek
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *   * Neither the name of the author nor the names of its contributors may be
 *     used to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "id3v2parser.h"


/**
 *
 */
static struct id3v2_frame_textinfo_s {
	char *id;
	char *info;
	char *text;
} id3v2_textinfo[] = {
    {"TIT1", 	"Content group:     ", 	NULL},
    {"TIT2", 	"Title:             ", 	NULL},
    {"TIT3", 	"Subtitle:          ", 	NULL},
    {"TALB", 	"Album:             ", 	NULL},
    {"TOAL", 	"Original album:    ",	NULL},
    {"TRCK", 	"Track number:      ",	NULL},
    {"TPOS", 	"Part of a set:     ",  NULL},
    {"TSST", 	"Set subtitle:      ",  NULL},
    {"TSRC", 	"ISRC:              ",  NULL},

    {"TPE1", 	"Lead artist:       ",  NULL},
    {"TPE2", 	"Band:              ",  NULL},
    {"TPE3", 	"Conductor:         ",  NULL},
    {"TPE4", 	"Interpreted:       ",  NULL},
    {"TOPE", 	"Orig. artist:      ",  NULL},
    {"TEXT", 	"Lyricist:          ",  NULL},
    {"TOLY", 	"Original lyricist: ",  NULL},
    {"TCOM", 	"Composer:          ",  NULL},
    {"TMCL", 	"Musician credits:  ",  NULL},
    {"TIPL", 	"Involved people:   ",  NULL},
    {"TENC", 	"Encoded by:        ",  NULL},

    {"TBPM", 	"BPM:               ",  NULL},
    {"TLEN", 	"Length:            ",  NULL},
    {"TKEY", 	"Initial key:       ",  NULL},
    {"TLAN", 	"Language:          ",  NULL},
    {"TCON", 	"Content type:      ",  NULL},
    {"TFLT", 	"File type:         ",  NULL},
    {"TMED", 	"Media type:        ",  NULL},
    {"TMOO", 	"Mood:              ",  NULL},

    {"TCOP", 	"Copyright message: ",  NULL},
    {"TPRO", 	"Produced notice:   ",  NULL},
    {"TPUB", 	"Publisher:         ",  NULL},
    {"TOWN", 	"File owner:        ",  NULL},
    {"TRSN", 	"Internet radio station name: ", NULL},
    {"TRSO", 	"Internet radio station owner: ",  NULL},

    {"TOFN", 	"Orig. filename:    ",  NULL},
    {"TDLY", 	"Playlist delay:    ",  NULL},
    {"TDEN", 	"Encoding time:     ",  NULL},
    {"TDOR", 	"Orig. release time:",  NULL},
    {"TDRC", 	"Recording time:    ",  NULL},
    {"TDRL", 	"Release time:      ",  NULL},
    {"TDTG", 	"Tagging time:      ",  NULL},
    {"TSSE", 	"SW/HW and settings used for encoding: ",  NULL},
    {"TSOA", 	"Album sort:        ",  NULL},
    {"TSOP", 	"Performer sort:    ",  NULL},
    {"TSOT", 	"Title sort:        ",  NULL},

    {NULL, 		NULL, 					NULL},
};

/**
 *
 */
static struct id3frame_other_info_s {
	char *lang;
	char *descr;
	char *text;
} id3v2_lyrics = {NULL, NULL, NULL};

/**
 *
 */
static struct id3frame_apic_type_s {
	uint8_t type;
	char *text;
	char *mime;
	char *descr;
	char *data;
	uint16_t flags;
} id3frame_apic_type[] = {
	{0x00, "other", 				NULL, NULL, NULL, 0},
	{0x01, "file icon", 			NULL, NULL, NULL, 0},
	{0x02, "other file icon", 		NULL, NULL, NULL, 0},
	{0x03, "cover front", 			NULL, NULL, NULL, 0},
	{0x04, "cover back", 			NULL, NULL, NULL, 0},
	{0x05, "leaflet page", 			NULL, NULL, NULL, 0},
	{0x06, "media", 				NULL, NULL, NULL, 0},
	{0x07, "soloist", 				NULL, NULL, NULL, 0},
	{0x08, "artist", 				NULL, NULL, NULL, 0},
	{0x09, "conductor", 			NULL, NULL, NULL, 0},
	{0x0A, "band", 					NULL, NULL, NULL, 0},
	{0x0B, "composer", 				NULL, NULL, NULL, 0},
	{0x0C, "lyricist", 				NULL, NULL, NULL, 0},
	{0x0D, "recording location", 	NULL, NULL, NULL, 0},
	{0x0E, "during recording", 		NULL, NULL, NULL, 0},
	{0x0F, "during performance", 	NULL, NULL, NULL, 0},
	{0x10, "movie screen capture", 	NULL, NULL, NULL, 0},
	{0x11, "bright coloured fish", 	NULL, NULL, NULL, 0},
	{0x12, "illustration", 			NULL, NULL, NULL, 0},
	{0x13, "band logotype", 		NULL, NULL, NULL, 0},
	{0x14, "publisher", 			NULL, NULL, NULL, 0},

	{0xff, "image", 				NULL, NULL, NULL, 0} // default value - not in specification
};



/* Main function - run program as './id3v2parser mp3_file_to_parse.mp3' */
int main(int argc, char *argv[]) {
	unsigned char *buffer;
	uint32_t buffer_len;
	uint16_t i;

	/* Program receives as its first argument name of the MP3 file */
	if(argc != 2)  {
		fprintf(stderr, "Wrong number of arguments - run program as '%s file.mp3'\n", argv[0]);
		return 1;
	}

	/* Read file and store binary data in the buffer */
	if(read_file(argv[1], &buffer, &buffer_len) == 1) {
		fprintf(stderr, "Problem while reading MP3 file %s has appeared\n", argv[1]);
		return 1;
	}

	/* Parse input file */
	if(parse_buffer(buffer, buffer_len) == 0) {
		/* Print info */
		printf("Textual information:\n");
		for(i=0; id3v2_textinfo[i].id; i++) {
			if(id3v2_textinfo[i].text){
				printf("%s %s\n", id3v2_textinfo[i].info, id3v2_textinfo[i].text);
			}
		}
		for(i=0; i < sizeof(id3frame_apic_type); i++) {
			if(id3frame_apic_type[i].data){
				printf("Picture:\n\t%s\n", id3frame_apic_type[i].mime);
				if(id3frame_apic_type[i].descr) {
					printf("\tdescription: %s\n", id3frame_apic_type[i].descr);
				}
				//TODO print into file
			}
		}
		if(id3v2_lyrics.text) {
			printf("----\nLyrics (language %s):\n%s\n----\n", id3v2_lyrics.lang, id3v2_lyrics.text);
		}
	}

	/* Free dynamically allocated memory */
	free(buffer);

	for(i=0; id3v2_textinfo[i].id; i++) {
		if(id3v2_textinfo[i].text){
			free(id3v2_textinfo[i].text);
		}
	}

	return 0;
}


int read_file(char * name, unsigned char ** p_buffer, uint32_t * p_len) {
	FILE *file;
	size_t result;


	// Open file in read binary mode
	file = fopen(name, "rb");
	if (file == NULL) {
		fprintf(stderr, "Error while opening file %s!\n", name);
		return 1;
	}

	// Find out length of the file
	fseek(file, 0, SEEK_END);
	*p_len = ftell(file);
	fseek(file, 0, SEEK_SET);

	// Allocate memory for an input buffer
	*p_buffer = (unsigned char *) malloc(*p_len);
	if (*p_buffer == NULL)	{
		fprintf(stderr, "Error while allocating memory for buffer!\n");
        fclose(file);
		return 1;
	}

	//Read content of the file and store it into the buffer
	result = fread(*p_buffer, 1, *p_len, file);
	if(result != *p_len) {
		fprintf(stderr, "Error while reading file!\n");
		fclose(file);
		return 1;
	}
	fclose(file);

	return 0;
}

/*
 *   +-----------------------------+
 *   |      Header (10 bytes)      |
 *   +-----------------------------+
 *   |       Extended Header       | - given by (header.flags & FLAG_ID3_EXTEND) bit
 *   | (variable length, OPTIONAL) |
 *   +-----------------------------+
 *   |   Frames (variable length)  |
 *   +-----------------------------+
 *   |           Padding           |
 *   | (variable length, OPTIONAL) |
 *   +-----------------------------+
 *   | Footer (10 bytes, OPTIONAL) | - given by (header.flags & FLAG_ID3_FOOTER) bit
 *   +-----------------------------+
 */

int parse_buffer(unsigned char *buffer, uint32_t buffer_len) {
	id3v2_header_t header;
	unsigned char *p_buff = buffer;
	int ret_code;

	printf("MP3 file length: %u\n", buffer_len);

	//Validate that there are enough data in buffer to parse
	if(buffer_len < HEADER_LEN) {
		fprintf(stderr, "Error - file is too small to include ID3 header (10 bytes)\n");
		return 1;
	}

	//ID3 tag is supposed to be at the beginning of the MP3 file.
	//In minor cases (I was not able to get an example) it can be elsewhere - but this has not been implemented.
	//Process Header
	ret_code = parse_id3v2_header(&p_buff, &header);
	if(ret_code != 0) {
		return 1;
	}

	print_id3v2_header(header); //Enable

	if(header.major_version != 4) {
		fprintf(stderr, "Cannot process ID3v2.%u tag. Only parsing of ID3v2.4 is implemented!\n", header.major_version);
		return 1;
	}

	//Process Extended Header (OPTIONAL)
	if(header.flags & FLAG_ID3_EXTEND) {
		// The extended header contains information that can provide further
		// insight in the structure of the tag, but is not vital to the correct
		// parsing of the tag information; hence the extended header is optional.
		skip_id3v2_extended_header(&p_buff);
	}

	//Validate that there are enough data in buffer to parse
	if(buffer_len < p_buff - buffer + header.size) {
		fprintf(stderr, "Error - file is too small to include ID3 tag (probably corrupted), as derived from ID3 header\n");
		return 1;
	}

	//Process Frames until size or Padding
	while((uint32_t) (p_buff - buffer) < header.size) {
#ifdef DEBUG
		printf("%u: ", (uint32_t) (p_buff - buffer));
#endif
		id3v2_frame_header_t frame_header;

		// Process Frame header
		ret_code = parse_id3v2_frame_header(&p_buff, &frame_header);
		if(ret_code != 0) {
			break;
		}
//#ifdef DEBUG
		print_id3v2_frame_header(frame_header);
//#endif

		// Process Frame body
		parse_id3v2_frame_body(&p_buff, frame_header);

		//printf("size: %u, current: %u\n", header.size, p_buff - buffer);
	}

	return 0;
}


int parse_id3v2_header(unsigned char **p_header_buff, id3v2_header_t* header) {
	uint8_t tmp_size[4];

#ifdef DEBUG
	print_hexa(*p_header_buff, 10);
#endif

	snprintf((char*) header->id, 4, (char*) *p_header_buff);
	*p_header_buff += 3;
	if(strcmp("ID3", (char *) header->id) != 0) {
		fprintf(stderr, "There is no ID3 tag in front of the file\n");
		return 1;
	}

	header->major_version = (uint8_t)*(*p_header_buff)++;
	header->revision_num = (uint8_t)*(*p_header_buff)++;
	header->flags = (uint8_t)*(*p_header_buff)++;

	memcpy(&tmp_size[0], *p_header_buff, 4);
	*p_header_buff += 4;
	header->size = ((tmp_size[0] & 0x7F) << 21) | ((tmp_size[1] & 0x7F) << 14) | ((tmp_size[2] & 0x7F) << 7) | (tmp_size[3] & 0x7F);

	return 0;
}


int skip_id3v2_extended_header(unsigned char **p_header_buff) {
	uint8_t tmp_size[4];
	uint32_t size;

#ifdef DEBUG
	print_hexa(*p_header_buff, 6);
#endif

	// Parse size of the extended header
	memcpy(&tmp_size[0], *p_header_buff, 4);
	*p_header_buff += 4;
	size = ((tmp_size[0] & 0x7F) << 21) | ((tmp_size[1] & 0x7F) << 14) | ((tmp_size[2] & 0x7F) << 7) | (tmp_size[3] & 0x7F);

	// Skip next 2 bytes of information
	*p_header_buff += 2;

	// Skip body of external header
	*p_header_buff += size;

	return 0;
}


int parse_id3v2_frame_header(unsigned char **p_header_buff, id3v2_frame_header_t* header) {
	uint8_t tmp_size[4];
	uint8_t tmp_flags[2];

#ifdef DEBUG
	print_hexa(*p_header_buff, 10);
#endif

	snprintf((char*) header->id, 5, (char*) *p_header_buff);
	*p_header_buff += 4;
	if(header->id[0] == (unsigned char) 0x00) {
		//printf("Seems that there is empty space:\n\t\t");
		//print_hexa(*p_header_buff, 50);
		// Frame is empty so the rest of ID3 tag does
		return 1;
	}

	memcpy(&tmp_size[0], *p_header_buff, 4);
	*p_header_buff += 4;
	header->size = ((tmp_size[0] & 0x7F) << 21) | ((tmp_size[1] & 0x7F) << 14) | ((tmp_size[2] & 0x7F) << 7) | (tmp_size[3] & 0x7F);

	tmp_flags[0] = (uint8_t)*(*p_header_buff)++;
	tmp_flags[1] = (uint8_t)*(*p_header_buff)++;
	header->flags = (tmp_flags[0] << 8) | tmp_flags[1];

	return 0;
}



int parse_id3v2_frame_body(unsigned char **p_header_buff, id3v2_frame_header_t header) {
	uint32_t i;
	uint32_t j;
	uint8_t encoding;
	uint8_t len;
	char * text;
	char * subtype;

#ifdef DEBUG
		printf("\t\t");
		print_hexa(*p_header_buff, header.size);
#endif

	i = 0;

	// Skip length info
	if(header.flags & FLAG_FR_LEN) {
		i += 4;
		//memcpy(&tmp_size[0], *p_header_buff, 4);
		//*p_header_buff += 4;
		//uint32_t size = ((tmp_size[0] & 0x7F) << 21) | ((tmp_size[1] & 0x7F) << 14) | ((tmp_size[2] & 0x7F) << 7) | (tmp_size[3] & 0x7F);
		//printf("Ext length: %u\n", size);
	}

	if(header.id[0] == 'T') { // Process 'Text information frame'
		for(j = 0; id3v2_textinfo[j].id; j++) {
			if(strcmp(id3v2_textinfo[j].id, (char*) header.id) == 0) {
				encoding = (uint8_t)*(*p_header_buff);
				if(encoding == ENC_UTF_8 || encoding == ENC_ISO_8859_1) {	//UTF-8 encoding or ISO-8859-1
					text = malloc(header.size);
					snprintf(text, header.size, (char*) *p_header_buff+1);
					id3v2_textinfo[j].text = text;
				}
				else {
					fprintf(stderr, "Decoding of encoding type %u is not supported (it is not typical to use it for ID3v2.4 tag)\n", encoding);
				}
				break;
			}
		}
	}
	else if(strcmp((char *) header.id, "USLT") == 0) { // Process 'Unsynchronised lyrics'
		encoding = (uint8_t)*(*p_header_buff+i++);
		if(encoding == ENC_UTF_8 || encoding == ENC_ISO_8859_1) {
			id3v2_lyrics.lang = malloc(4);
			snprintf((char*) id3v2_lyrics.lang, 4, (char*) *p_header_buff+i);
			i += 3;

			len = strlen((char *) *p_header_buff+i);
			id3v2_lyrics.descr = malloc(len + 1);
			snprintf((char *) id3v2_lyrics.descr, len + 1, (char*) *p_header_buff+i);
			i += len + 1;

			text = malloc(header.size-i+1);
			snprintf(text, header.size-i+1, (char*) *p_header_buff+i);

			id3v2_lyrics.text = malloc(header.size-i+1);
			snprintf((char *) id3v2_lyrics.text, header.size-i+1, (char*) *p_header_buff+i);
		}
		else {
			fprintf(stderr, "Not able to decode USLT tag\n");
		}
	}
	else if(strcmp((char *) header.id, "APIC") == 0) { // Process 'Attached picture'
		encoding = (uint8_t)*(*p_header_buff+i++);
		//printf("Enc: %u\n", encoding);
		len = strlen((char *) *p_header_buff+i);
		char * mime = malloc(len + 1);
		snprintf((char *) mime, len + 1, (char*) *p_header_buff+i);
		i += len + 1;
		//printf("MIME: %s\n", mime);
		//Process MIME subtype - get the second
		strtok_r (mime, "/", &subtype);

		uint8_t type = (uint8_t)*(*p_header_buff+i++);
		//printf("Type: %u\n", type);

		for(j = 0; j < sizeof(id3frame_apic_type)-1; j++) {
			if(type == id3frame_apic_type[j].type) {
				break;
			}
		}
		id3frame_apic_type[j].mime = mime;
		len = strlen((char *) *p_header_buff+i);
		id3frame_apic_type[j].descr = malloc(len + 1);
		snprintf(id3frame_apic_type[j].descr, len + 1, (char*) *p_header_buff+i);
		i += len + 1;
		id3frame_apic_type[j].data = malloc(header.size - i);
		snprintf(id3frame_apic_type[j].data, header.size - i, (char*) *p_header_buff+i);

//		if(strlen(descr) > 0) {
//			printf("Descr: %s\n", descr);
//		}

		// Print image into file
		/*FILE *f_image;

		char * filename = malloc(50);
		snprintf(filename, 50, "%s.%s", (char *) id3frame_apic_type[j].text, subtype);
		f_image = fopen(filename, "wb");
		printf("Writing file %s\n", filename);
		for(j = 0; j < header.size - i; j++) {
			fwrite(*p_header_buff+i+j, 1 , 1, f_image);
			if(header.flags & FLAG_FR_UNSYNC) { // if unsynchronization occurs
				if((*(*p_header_buff+i+j) == 0xff) && (*(*p_header_buff+i+j+1) == 0x00)) {
					j++;
				}
			}
		}
		fclose(f_image);*/
		//free(filename);
	}
#ifdef DEBUG
	else {
		fprintf(stderr, "Tag id %s skipped\n", header.id);
	}
#endif

	// Move pointer to the next frame header
	*p_header_buff += header.size;

	return 0;
}


void print_hexa(unsigned char *buffer, size_t len) {
	size_t i;
	for(i = 0; i<len; i++) {
		printf("%02x ", buffer[i]);
	}
	printf("\n");
}


void print_id3v2_header(id3v2_header_t header) {
	printf("ID3 header:\n");
	printf("\tVersion: ID3v2.%u.%u\n", header.major_version, header.revision_num);
	printf("\tFlags: %u\n", header.flags);
	if(header.flags > 0) {
		printf("\t\tUnsynchronisation: %i,\n", (header.flags & FLAG_ID3_UNSYNC) != 0);
		printf("\t\tExtended header: %i,\n", (header.flags & FLAG_ID3_EXTEND) != 0);
		printf("\t\tExperimental indicator: %i,\n", (header.flags & FLAG_ID3_EXPER) != 0);
		printf("\t\tFooter present: %i\n", (header.flags & FLAG_ID3_FOOTER) != 0);
	}
	printf("\tSize: %u\n", header.size);
}


void print_id3v2_frame_header(id3v2_frame_header_t header) {
	printf("Frame ID: %s\n", header.id);
	printf("\tSize: %u\n", header.size);
	printf("\tFlags: %u\n", header.flags);
	if(header.flags > 0) {
		printf("\t\tTag alter preservation: %i,\n", (header.flags & FLAG_FR_TAG) != 0);
		printf("\t\tFile alter preservation: %i,\n", (header.flags & FLAG_FR_FILE) != 0);
		printf("\t\tRead only: %i,\n", (header.flags & FLAG_FR_READ) != 0);
		printf("\t\tGrouping identity: %i,\n", (header.flags & FLAG_FR_GROUP) != 0);
		printf("\t\tCompression: %i,\n", (header.flags & FLAG_FR_COMP) != 0);
		printf("\t\tEncryption: %i,\n", (header.flags & FLAG_FR_ENCR) != 0);
		printf("\t\tUnsynchronisation: %i,\n", (header.flags & FLAG_FR_UNSYNC) != 0);
		printf("\t\tData length indicator: %i,\n", (header.flags & FLAG_FR_LEN) != 0);
	}
}



