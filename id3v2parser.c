/*
 ============================================================================
 Name        : id3v2parser.c
 Author      : Martin Rabek
 Version     :
 Copyright   : TODO
 Description : TODO
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "id3v2parser.h"

// Comment the following to the DEBUG info
//#define DEBUG

static struct id3v2_frame_textinfo_s {
	char *id;
	char *info;
	uint8_t empty;
	char *text;
} id3v2_frame_textinfo[] = {
    {"TIT1", 	"Content group:     ", 	0, NULL},
    {"TIT2", 	"Title:             ", 	0, NULL},
    {"TIT3", 	"Subtitle:          ", 	0, NULL},
    {"TALB", 	"Album:             ", 	0, NULL},
    {"TOAL", 	"Original album:    ",	0, NULL},
    {"TRCK", 	"Track number:      ",	0, NULL},
    {"TPOS", 	"Part of a set:     ",  0, NULL},
    {"TSST", 	"Set subtitle:      ",  0, NULL},
    {"TSRC", 	"ISRC:              ",  0, NULL},

    {"TPE1", 	"Lead artist:       ",  0, NULL},
    {"TPE2", 	"Band:              ",  0, NULL},
    {"TPE3", 	"Conductor:         ",  0, NULL},
    {"TPE4", 	"Interpreted:       ",  0, NULL},
    {"TOPE", 	"Orig. artist:      ",  0, NULL},
    {"TEXT", 	"Lyricist:          ",  0, NULL},
    {"TOLY", 	"Original lyricist: ",  0, NULL},
    {"TCOM", 	"Composer:          ",  0, NULL},
    {"TMCL", 	"Musician credits:  ",  0, NULL},
    {"TIPL", 	"Involved people:   ",  0, NULL},
    {"TENC", 	"Encoded by:        ",  0, NULL},

    {"TBPM", 	"BPM:               ",  0, NULL},
    {"TLEN", 	"Length:            ",  0, NULL},
    {"TKEY", 	"Initial key:       ",  0, NULL},
    {"TLAN", 	"Language:          ",  0, NULL},
    {"TCON", 	"Content type:      ",  0, NULL},
    {"TFLT", 	"File type:         ",  0, NULL},
    {"TMED", 	"Media type:        ",  0, NULL},
    {"TMOO", 	"Mood:              ",  0, NULL},

    {"TCOP", 	"Copyright message: ",  0, NULL},
    {"TPRO", 	"Produced notice:   ",  0, NULL},
    {"TPUB", 	"Publisher:         ",  0, NULL},
    {"TOWN", 	"File owner:        ",  0, NULL},
    {"TRSN", 	"Internet radio station name: ",  0, NULL},
    {"TRSO", 	"Internet radio station owner: ",  0, NULL},

    {"TOFN", 	"Orig. filename:    ",  0, NULL},
    {"TDLY", 	"Playlist delay:    ",  0, NULL},
    {"TDEN", 	"Encoding time:     ",  0, NULL},
    {"TDOR", 	"Orig. release time:",  0, NULL},
    {"TDRC", 	"Recording time:    ",  0, NULL},
    {"TDRL", 	"Release time:      ",  0, NULL},
    {"TDTG", 	"Tagging time:      ",  0, NULL},
    {"TSSE", 	"SW/HW and settings used for encoding: ",  0, NULL},
    {"TSOA", 	"Album sort:        ",  0, NULL},
    {"TSOP", 	"Performer sort:    ",  0, NULL},
    {"TSOT", 	"Title sort:        ",  0, NULL},

    {NULL, 		NULL, 					0, NULL},
};


// TODO http://forum.dbpoweramp.com/showthread.php?18418-id3v2-4-tag-support
// Problem with readability of id3v2.4 by programs (e.g. WMP stores changed 2.4 file as 2.3)
	// The bitorder in ID3v2 is most significant bit first (MSB). The byteorder in multibyte numbers is most significant byte
	// first (e.g. $12345678 would be encoded $12 34 56 78), also known as big endian and network byte order.

int main(int argc, char *argv[]) {
	unsigned char *buffer;
	unsigned long buffer_len;
	uint16_t i;

	// Check number of arguments
	if(argc < 2) {
		fprintf(stderr, "Missing argument - run program as '%s file.mp3'\n", argv[0]);
		return 1;
	}
	else if(argc > 2) {
		fprintf(stderr, "Too many arguments - run program as '%s file.mp3'\n", argv[0]);
		return 1;
	}

	// Read file and store binary data in the buffer
	if(read_file(argv[1], &buffer, &buffer_len) == 1) {
		fprintf(stderr, "Problem while reading MP3 file %s has appeared\n", argv[1]);
		return 1;
	}

	// Parse input file
	parse_buffer(buffer, buffer_len);

	// Print info
	printf("\n----\nParsed info (will be printed into a file):\n");
	for(i=0; id3v2_frame_textinfo[i].id; i++) {
		if(id3v2_frame_textinfo[i].empty == 1){
			printf("%s %s\n", id3v2_frame_textinfo[i].info, id3v2_frame_textinfo[i].text);
		}
	}

	// Free dynamically allocated memory
	for(i=0; id3v2_frame_textinfo[i].id; i++) {
		if(id3v2_frame_textinfo[i].empty == 1){
			free(id3v2_frame_textinfo[i].text);
		}
	}
	free(buffer);

	return 0;
}


int read_file(char * name, unsigned char ** p_buffer, unsigned long * p_len) {
	FILE *file;

	// Open file in read binary mode
	file = fopen(name, "rb");
	if (!file) {
		fprintf(stderr, "Error while opening file %s!\n", name);
		return 1;
	}

	// Find out length of the file
	fseek(file, 0, SEEK_END);
	*p_len = ftell(file);
	fseek(file, 0, SEEK_SET);

	// Allocate memory for an input buffer
	*p_buffer = (unsigned char *) malloc(*p_len);
	if (!*p_buffer)	{
		fprintf(stderr, "Error while allocating memory for buffer!\n");
        fclose(file);
		return 1;
	}

	//Read content of the file and store it into the buffer
	fread(*p_buffer, *p_len, 1, file);
	//TODO - Should I be interested in short read? As shown at the seminar...
	fclose(file);

	return 0;
}

/*
 *
 *   +-----------------------------+
 *   |      Header (10 bytes)      |
 *   +-----------------------------+
 *   |       Extended Header       | - given by B_HEAD_EXTEND(flags) bit
 *   | (variable length, OPTIONAL) |
 *   +-----------------------------+
 *   |   Frames (variable length)  |
 *   +-----------------------------+
 *   |           Padding           |
 *   | (variable length, OPTIONAL) |
 *   +-----------------------------+
 *   | Footer (10 bytes, OPTIONAL) | - given by B_HEAD_FOOTER(flags) bit
 *   +-----------------------------+
 */

int parse_buffer(unsigned char *buffer, unsigned long buffer_len) {
	id3v2_header_t header;
	unsigned char *p_buff = buffer;
	int ret_code;

	printf("MP3 file length: %lu\n", buffer_len);

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
		// TODO later
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
#ifdef DEBUG
		print_id3v2_frame_header(frame_header); //Enable
#endif

		// Process Frame body
		parse_id3v2_frame_body(&p_buff, frame_header);
	}

	//Process Footer (OPTIONAL)
	if(header.flags & FLAG_ID3_FOOTER) {
		// Not implemented due to:
		// a. it is copy of the ID3 header and brings no new information,
		// b. it is not used in my testing MP3 files (rarely used),
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




int parse_id3v2_frame_header(unsigned char **p_header_buff, id3v2_frame_header_t* header) {
	uint8_t tmp_size[4];
	uint8_t tmp_flags[2];

#ifdef DEBUG
	print_hexa(*p_header_buff, 10);
#endif

	snprintf((char*) header->id, 5, (char*) *p_header_buff);
	*p_header_buff += 4;
	if(header->id[0] == (unsigned char) 0x00) {
		//Frame is empty\n");
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


#define ENC_ISO_8859_1 0x00
#define ENC_UTF_8 0x03

int parse_id3v2_frame_body(unsigned char **p_header_buff, id3v2_frame_header_t header) {
	uint16_t i;
	uint8_t encoding;
	char *text;

#ifdef DEBUG
		printf("\t\t");
		print_hexa(*p_header_buff, header.size);
#endif

	// Process 'Text information frame'
	if(header.id[0] == 'T') {
		for(i = 0; id3v2_frame_textinfo[i].id; i++) {
			if(strcmp(id3v2_frame_textinfo[i].id, (char*) header.id) == 0) {
				encoding = (uint8_t)*(*p_header_buff);
				if(encoding == ENC_UTF_8 || encoding == ENC_ISO_8859_1) {	//UTF-8 encoding or ISO-8859-1
					text = malloc(header.size);
					snprintf(text, header.size, (char*) *p_header_buff+1);
					id3v2_frame_textinfo[i].empty = 1;
					id3v2_frame_textinfo[i].text = text;
				}
				else {
					fprintf(stderr, "Decoding of encoding type %u is not supported (it is not typical to use it for ID3v2.4 tag)\n", encoding);
				}
				break;
			}
		}
	}
	else if(strcmp((char *) header.id, "USLT") == 0) {
		encoding = (uint8_t)*(*p_header_buff);
		if(encoding == ENC_UTF_8) {
			// TODO - add USLT to the tag

			unsigned char language[4];
			uint8_t i = 1;
			snprintf((char*) language, 4, (char*) *p_header_buff+i);
			printf("Begin of lyrics:\n\tLanguage: %s\n", language);
			i += 3;
			unsigned char * descr;
			uint8_t len = strlen((char *) *p_header_buff+i);
			//printf("len %u\n", len);

			descr = malloc(len + 1);
			snprintf((char *) descr, len + 1, (char*) *p_header_buff+i);
			printf("Description: %s\n", descr);
			i += len + 1;

			text = malloc(header.size-i+1);
			snprintf(text, header.size-i+1, (char*) *p_header_buff+i);
			printf("Lyrics: %s\n", text);

			//TODO - free memory in main function
			free(descr);
			free(text);
		}
		else {
			fprintf(stderr, "Not able to decode %u encoded USLT tag\n", encoding);
		}
	}
	else if(strcmp((char *) header.id, "APIC") == 0) {
		// TODO - not working yet
		/*printf("\t\t");
		print_hexa(*p_header_buff, header.size);

		// TODO picture
		uint8_t len;
		uint8_t i = 0;
		encoding = (uint8_t)*(*p_header_buff);
		printf("enc %u\n", encoding);
		i++;
		len = strlen((char *) *p_header_buff+i);
		//printf("i %u, len %u\n", i, len);

		// *char * mime = malloc(len + 1);
		snprintf((char *) mime, len + 1, (char*) *p_header_buff+i);
		printf("MIME: %s\n", mime);
		i += len + 1; // * /
		//printf("i %u, len %u\n", i, len);

		uint8_t type = (uint8_t)*(*p_header_buff+i);
		printf("Type: %u\n", type);
		i++;

		char * descr = malloc(len + 1);
		snprintf((char *) descr, len + 1, (char*) *p_header_buff+i);
		printf("Descr: %s\n", descr);
		i += len + 1;

		printf("Binary data:\t");
		print_hexa(*p_header_buff+i, header.size-i);*/
	}
	else {
#ifdef DEBUG
		fprintf(stderr, "Tag id %s skipped\n", header.id);
#endif
	}

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



