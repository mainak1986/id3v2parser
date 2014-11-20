/*
 *  id3v2parser - program to parse ID3v2.4 tags
 *
 * 	This program can read ID3 tags but only in version 2.4, according to the
 * 	latest specification. There were some troubles with version 2.3, so it
 * 	has not been implemented.
 *
 * 	Parser is able to parse following types of data:
 * 	  - textual information,
 * 	  - unsychronized lyrics,
 * 	  - pictures within tag.
 * 	Other frames which are not parsed, are skipped. In the program's output
 * 	you can see four-char frame IDs.
 *
 *  How to build: 'gcc -std=c11 -Wall -Wextra id3v2parser.c -o id3v2parser'
 *
 *  How to run: './id3v2parser mp3_file_to_parse.mp3'
 *
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


/** Structure for textual information */
static struct id3v2_frame_textinfo_s {
	char *id;				/**< frame ID code */
	char *info;				/**< text corresponding to the frame ID code */
	char *text;				/**< text from the frame body */
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

    {NULL, 		NULL, 					NULL}
};

/** Structure for lyrics information */
static struct id3frame_other_info_s {
	char *lang;				/**< language of the lyrics */
	char *descr;			/**< description of the lyrics */
	char *text;				/**< lyrics text */
} id3v2_lyrics = {NULL, NULL, NULL};

/** Structure for pictures information */
static struct id3frame_apic_type_s {
	uint8_t type;			/**< type of the image */
	char *text;				/**< text corresponding to the type */
	char *mime;				/**< mime type of the image */
	char *descr;			/**< description of the image */
	unsigned char *data;	/**< binary data of the image */
	uint8_t flags;			/**< flags of the frame header */
	uint32_t len;			/**< length of store binary data */
} id3frame_apic_type[] = {
	{0x00, "other", 				NULL, NULL, NULL, 0, 0},
	{0x01, "file icon", 			NULL, NULL, NULL, 0, 0},
	{0x02, "other file icon", 		NULL, NULL, NULL, 0, 0},
	{0x03, "cover front", 			NULL, NULL, NULL, 0, 0},
	{0x04, "cover back", 			NULL, NULL, NULL, 0, 0},
	{0x05, "leaflet page", 			NULL, NULL, NULL, 0, 0},
	{0x06, "media", 				NULL, NULL, NULL, 0, 0},
	{0x07, "soloist", 				NULL, NULL, NULL, 0, 0},
	{0x08, "artist", 				NULL, NULL, NULL, 0, 0},
	{0x09, "conductor", 			NULL, NULL, NULL, 0, 0},
	{0x0A, "band", 					NULL, NULL, NULL, 0, 0},
	{0x0B, "composer", 				NULL, NULL, NULL, 0, 0},
	{0x0C, "lyricist", 				NULL, NULL, NULL, 0, 0},
	{0x0D, "recording location", 	NULL, NULL, NULL, 0, 0},
	{0x0E, "during recording", 		NULL, NULL, NULL, 0, 0},
	{0x0F, "during performance", 	NULL, NULL, NULL, 0, 0},
	{0x10, "movie screen capture", 	NULL, NULL, NULL, 0, 0},
	{0x11, "bright coloured fish", 	NULL, NULL, NULL, 0, 0},
	{0x12, "illustration", 			NULL, NULL, NULL, 0, 0},
	{0x13, "band logotype", 		NULL, NULL, NULL, 0, 0},
	{0x14, "publisher", 			NULL, NULL, NULL, 0, 0},

	{0xff, NULL, 					NULL, NULL, NULL, 0, 0}
};


int main(int argc, char *argv[]) {
	unsigned char *buffer;
	uint32_t buffer_len;

	/* Program receives as its first argument name of the MP3 file */
	if(argc != 2)  {
		fprintf(stderr, "Wrong number of arguments - run program as '%s file.mp3'!\n", argv[0]);
		return 1;
	}

	/* Read file and store binary data in the buffer */
	if(read_file(argv[1], &buffer, &buffer_len) == 1) {
		fprintf(stderr, "Error while reading MP3 file %s has appeared!\n", argv[1]);
		return 1;
	}

	/* Parse input file */
	if(parse_buffer(buffer, buffer_len) != 0) {
		fprintf(stderr, "Error while parsing input buffer occurred!\n");
		deallocate_memory(buffer); /* Free dynamically allocated memory */
		return 1;
	}

	/* Write parsed data into file(s) */
	if(write_parsed_data(argv[1]) != 0) {
		fprintf(stderr, "Error while writing parsed data into file(s) occurred!\n");
		deallocate_memory(buffer); /* Free dynamically allocated memory */
		return 1;
	}

	/* Free dynamically allocated memory */
	deallocate_memory(buffer);

	return 0;
}


int read_file(char * name, unsigned char ** p_buffer, uint32_t * p_len) {
	FILE *file;

	/* Open file in read binary mode */
	file = fopen(name, "rb");
	if (file == NULL) {
		fprintf(stderr, "Error while opening file %s!\n", name);
		return 1;
	}

	/* Find out length of the file */
	fseek(file, 0, SEEK_END);
	*p_len = ftell(file);
	fseek(file, 0, SEEK_SET);

	/* Allocate memory for an input buffer */
	*p_buffer = (unsigned char *) malloc(*p_len);
	if (*p_buffer == NULL)	{
		fprintf(stderr, "Error while allocating memory for buffer!\n");
        fclose(file);
		return 1;
	}

	/* Read content of the file and store it into the buffer */
	if(fread(*p_buffer, 1, *p_len, file) != *p_len) {
		fprintf(stderr, "Error while reading file!\n");
		fclose(file);
		return 1;
	}
	fclose(file);

	return 0;
}


int parse_buffer(unsigned char *buffer, uint32_t buffer_len) {
	id3v2_header_t header;
	unsigned char *p_buff = buffer;

	/* Validate that there are enough data in buffer to parse */
	printf("MP3 file length: %u\n", buffer_len);
	if(buffer_len < HEADER_LEN) {
		fprintf(stderr, "Error - file is too small to include ID3 header (10 bytes)\n");
		return 1;
	}

	/* In this implementation, ID3 tag is supposed to be at the beginning of the MP3 file */
	if(parse_id3v2_header(&p_buff, &header) == 1) {
		fprintf(stderr, "Error - missing ID3 tag so it cannot be parsed\n");
		return 1;
	}

	/* Print ID3 tag header information */
	print_id3v2_header(header);

	if(header.major_version != 4) {
		fprintf(stderr, "Cannot process ID3v2.%u tag. Only parsing of ID3v2.4 is implemented!\n", header.major_version);
		return 1;
	}

	/* Process Extended Header (OPTIONAL) */
	if(header.flags & FLAG_ID3_EXTEND) {
		skip_id3v2_extended_header(&p_buff);
	}

	/* Validate that there are enough data in buffer to parse */
	if(buffer_len < p_buff - buffer + header.size) {
		fprintf(stderr, "Error - file is too small to include ID3 tag (probably corrupted), as derived from ID3 header\n");
		return 1;
	}

	/* Process frames until size or padding */
	while((uint32_t) (p_buff - buffer) < header.size) {
#ifdef DEBUG
		printf("%u: ", (uint32_t) (p_buff - buffer));
#endif
		id3v2_frame_header_t frame_header;

		/* Process frame header */
		if(parse_id3v2_frame_header(&p_buff, &frame_header) == 1) {
			// Frame is empty but frame size is not over - padding is here
			break;
		}

		/* Print ID3 frame header information */
		print_id3v2_frame_header(frame_header);

		/* Process frame body */
		if(parse_id3v2_frame_body(&p_buff, frame_header) != 0) {
			fprintf(stderr, "Error while parsing ID3 frame body of ID %s\n", frame_header.id);
			return 1;
		}
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

	/* Parse size of the extended header */
	memcpy(&tmp_size[0], *p_header_buff, 4);
	*p_header_buff += 4;
	size = ((tmp_size[0] & 0x7F) << 21) | ((tmp_size[1] & 0x7F) << 14) | ((tmp_size[2] & 0x7F) << 7) | (tmp_size[3] & 0x7F);

	/* Skip next 2 bytes of information */
	*p_header_buff += 2;

	/* Skip body of external header */
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
		/* Frame is empty so the rest of ID3 tag does */
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
	uint32_t len;
	uint8_t type;
	uint8_t found;

#ifdef DEBUG
		printf("\t\t");
		print_hexa(*p_header_buff, header.size);
#endif

	i = 0;

	/* Skip length info - not particularly useful for parsing */
	if(header.flags & FLAG_FR_LEN) {
		i += 4;
	}

	if(header.id[0] == 'T') { /* Process 'Text information frame' */
		/* Select id3v2_textinfo[j] to store the parsed data */
		for(j = 0; id3v2_textinfo[j].id; j++) {
			if(strcmp(id3v2_textinfo[j].id, (char*) header.id) == 0) {
				encoding = (uint8_t)*(*p_header_buff+i++);
				if(encoding == ENC_UTF_8 || encoding == ENC_ISO_8859_1) { /* UTF-8 encoding or ISO-8859-1 */
					id3v2_textinfo[j].text = malloc(header.size);

					/* strncpy is preferred to use to snprintf because string read is not terminated by '\0' */
					strncpy(id3v2_textinfo[j].text, (char*) *p_header_buff+i, header.size-1);
					id3v2_textinfo[j].text[header.size-1] = '\0';

					//len = snprintf(id3v2_textinfo[j].text, header.size, (char*) *p_header_buff+i);
				}
				else {
					fprintf(stderr, "Decoding of encoding type %u is not supported (it is not typical to use it for ID3v2.4 tag)\n", encoding);
				}
				break;
			}
		}
	}
	else if(strcmp((char *) header.id, "USLT") == 0) { /* Process 'Unsynchronised lyrics' */
		encoding = (uint8_t)*(*p_header_buff+i++);
		if(encoding == ENC_UTF_8 || encoding == ENC_ISO_8859_1) {
			id3v2_lyrics.lang = malloc(4);
			snprintf(id3v2_lyrics.lang, 4, (char*) *p_header_buff+i);
			i += 3;

			len = strlen((char *) *p_header_buff+i);
			id3v2_lyrics.descr = malloc(len + 1);
			snprintf((char *) id3v2_lyrics.descr, len + 1, (char*) *p_header_buff+i);
			i += len + 1;

			id3v2_lyrics.text = malloc(header.size-i+1);
			snprintf(id3v2_lyrics.text, header.size-i+1, (char*) *p_header_buff+i);
		}
		else {
			fprintf(stderr, "Not able to decode USLT tag\n");
		}
	}
	else if(strcmp((char *) header.id, "APIC") == 0) { /* Process 'Attached picture' */
		encoding = (uint8_t)*(*p_header_buff+i++);
		len = strlen((char *) *p_header_buff+i);
		type = (uint8_t)*(*p_header_buff+i+len+1); /* Read 'type' first (is after mime) */

		/* Select id3frame_apic_type[j] to store the parsed data */
		found = 0;
		for(j = 0; id3frame_apic_type[j].type != 0xff; j++) {
			if(type == id3frame_apic_type[j].type) {
				found = 1;
				break;
			}
		}
		if(found) {
			id3frame_apic_type[j].mime = malloc(len + 1);
			snprintf(id3frame_apic_type[j].mime, len + 1, (char*) *p_header_buff+i);
			i += len + 1 + 1; /* Now skip 'type' because it is already read */

			len = strlen((char *) *p_header_buff+i);
			id3frame_apic_type[j].descr = malloc(len + 1);
			snprintf(id3frame_apic_type[j].descr, len + 1, (char*) *p_header_buff+i);
			i += len + 1;

			len = header.size - i;
			id3frame_apic_type[j].data = malloc(len);
			memset(id3frame_apic_type[j].data, 0, len);
			memcpy(id3frame_apic_type[j].data, *p_header_buff+i, len);
			id3frame_apic_type[j].len = len;
			id3frame_apic_type[j].flags = header.flags;
		}
	}
#ifdef DEBUG
	else {
		fprintf(stderr, "Tag id %s skipped\n", header.id);
	}
#endif

	/* Move pointer to the beginning of the next frame header */
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


int write_parsed_data(char * orig_name) {
	uint32_t i;
	uint32_t j;
	uint16_t len;
	FILE *p_file;
	FILE *p_file_image;
	char *filename;
	char *filename_image;
	char *extension;

	/* Write textual information from ID3 tag */
	len = strlen(orig_name) + strlen(".tag.txt") + 1;
	filename = malloc(len);
	snprintf(filename, len, "%s%s", orig_name, ".tag.txt");

	p_file = fopen(filename, "wb");
	if(p_file == NULL) {
		fprintf(stderr, "Error while opening file %s to write!\n", filename);
		free(filename);
		return 1;
	}

	fprintf(p_file, "Textual information parsed from file %s:\n", orig_name);
	if(ferror (p_file)) {
		fprintf(stderr, "Error while writing into file %s!\n", filename);
		fclose(p_file);
		free(filename);
		return 1;
	}
	for(i=0; id3v2_textinfo[i].id; i++) {
		if(id3v2_textinfo[i].text){
			fprintf(p_file, "\t%s %s\n", id3v2_textinfo[i].info, id3v2_textinfo[i].text);
			if(ferror (p_file)) {
				fprintf(stderr, "Error while writing into file %s!\n", filename);
				fclose(p_file);
				return 1;
			}
		}
	}

	for(i=0; id3frame_apic_type[i].type != 0xff; i++) {
		/* Write picture information from ID3 tag */
		if(id3frame_apic_type[i].data){
			len = sizeof(id3frame_apic_type[i].mime);
			extension = malloc(len);
			memset(extension, 0, len);
			sscanf(id3frame_apic_type[i].mime, "%*[^/],%s", extension);

			len = sizeof(orig_name) + sizeof(id3frame_apic_type[i].text) + sizeof(extension) + 3;
			filename_image = malloc(len);
			snprintf(filename_image, len, "%s.%s.%s", orig_name, id3frame_apic_type[i].text, extension);

			p_file_image = fopen(filename_image, "wb");
			unsigned char *p_data = id3frame_apic_type[i].data;
			for(j = 0; j < id3frame_apic_type[i].len; j++) {
				fwrite(p_data+j, 1 , 1, p_file_image);
				if(id3frame_apic_type[i].flags & FLAG_FR_UNSYNC) { // if unsynchronization occurs
					if((*(p_data+j) == 0xff) && (*(p_data+j+1) == 0x00)) {
						j++;
					}
				}
			}
			fclose(p_file_image);


			fprintf(p_file, "Picture:\n\t%s\n", id3frame_apic_type[i].mime);
			if(id3frame_apic_type[i].descr) {
				fprintf(p_file, "\tdescription: %s\n", id3frame_apic_type[i].descr);
			}
			fprintf(p_file, "\tpicture is stored in file %s\n", filename_image);

			free(extension);
			free(filename_image);
		}
	}

	if(id3v2_lyrics.text) {
		fprintf(p_file, "Lyrics:\n\tLanguage: %s\n%s\n", id3v2_lyrics.lang, id3v2_lyrics.text);
	}
	if(ferror (p_file)) {
		fprintf(stderr, "Error while writing into file %s!\n", filename);
		fclose(p_file);
		return 1;
	}

	printf("\nParsed ID3 tag textual frames written into file %s\n", filename);
	fclose(p_file);
	free(filename);

	return 0;
}


void deallocate_memory(unsigned char *buffer) {
	uint16_t i;

	/* Free memory for buffer of input MP3 file */
	free(buffer);

	/* Free memory for each id3v2_textinfo.text */
	for(i=0; id3v2_textinfo[i].id; i++) {
		if(id3v2_textinfo[i].text){
			free(id3v2_textinfo[i].text);
		}
	}
	printf("\n");

	/* Free memory for id3v2_lyrics items */
	if(id3v2_lyrics.descr) {
		free(id3v2_lyrics.descr);
	}
	if(id3v2_lyrics.text) {
		free(id3v2_lyrics.text);
	}

	/* Free memory for id3frame_apic_type items */
	for(i=0; id3frame_apic_type[i].type != 0xff; i++) {
		if(id3frame_apic_type[i].mime){
			free(id3frame_apic_type[i].mime);
		}
		if(id3frame_apic_type[i].descr){
			free(id3frame_apic_type[i].descr);
		}
		if(id3frame_apic_type[i].data){
			free(id3frame_apic_type[i].data);
		}
	}
}




