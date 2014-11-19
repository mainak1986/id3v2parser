#ifndef ID3V2PARSER_H_
#define ID3V2PARSER_H_

#define HEADER_LEN 10

/*
   ID3v2 header

   The first part of the ID3v2 tag is the 10 byte tag header, laid out
   as follows:

     ID3v2/file identifier      "ID3"
     ID3v2 version              $04 00
     ID3v2 flags                %abcd0000
     ID3v2 size             4 * %0xxxxxxx

   The first three bytes of the tag are always "ID3", to indicate that
   this is an ID3v2 tag, directly followed by the two version bytes. The
   first byte of ID3v2 version is its major version, while the second
   byte is its revision number. In this case this is ID3v2.4.0. All
   revisions are backwards compatible while major versions are not. If
   software with ID3v2.4.0 and below support should encounter version
   five or higher it should simply ignore the whole tag. Version or
   revision will never be $FF.

   The version is followed by the ID3v2 flags field, of which currently
   four flags are used.

   All the other flags MUST be cleared. If one of these undefined flags
   are set, the tag might not be readable for a parser that does not
   know the flags function.

   The ID3v2 tag size is stored as a 32 bit synchsafe integer (section
   6.2), making a total of 28 effective bits (representing up to 256MB).

   The ID3v2 tag size is the sum of the byte length of the extended
   header, the padding and the frames after unsynchronisation. If a
   footer is present this equals to ('total size' - 20) bytes, otherwise
   ('total size' - 10) bytes.

   An ID3v2 tag can be detected with the following pattern:
     $49 44 33 yy yy xx zz zz zz zz
   Where yy is less than $FF, xx is the 'flags' byte and zz is less than
   $80.
 */
typedef struct id3v2_header_s {
	unsigned char id[4];
	uint8_t major_version;
	uint8_t revision_num;
	uint8_t flags;
	uint32_t size;
} id3v2_header_t;

/*
   a - Unsynchronisation

     Bit 7 in the 'ID3v2 flags' indicates whether or not
     unsynchronisation is applied on all frames (see section 6.1 for
     details); a set bit indicates usage.
 */
#define FLAG_ID3_UNSYNC 0x80
/*
   b - Extended header

     The second bit (bit 6) indicates whether or not the header is
     followed by an extended header. The extended header is described in
     section 3.2. A set bit indicates the presence of an extended
     header.
 */
#define FLAG_ID3_EXTEND 0x40
/*
   c - Experimental indicator

     The third bit (bit 5) is used as an 'experimental indicator'. This
     flag SHALL always be set when the tag is in an experimental stage.
 */
#define FLAG_ID3_EXPER 0x20
/*
   d - Footer present

     Bit 4 indicates that a footer (section 3.4) is present at the very
     end of the tag. A set bit indicates the presence of a footer.
 */
#define FLAG_ID3_FOOTER 0x10



/*
3.2. Extended header

   The extended header contains information that can provide further
   insight in the structure of the tag, but is not vital to the correct
   parsing of the tag information; hence the extended header is
   optional.

     Extended header size   4 * %0xxxxxxx
     Number of flag bytes       $01
     Extended Flags             $xx

   Where the 'Extended header size' is the size of the whole extended
   header, stored as a 32 bit synchsafe integer. An extended header can
   thus never have a size of fewer than six bytes.

   The extended flags field, with its size described by 'number of flag
   bytes', is defined as:

     %0bcd0000

   Each flag that is set in the extended header has data attached, which
   comes in the order in which the flags are encountered (i.e. the data
   for flag 'b' comes before the data for flag 'c'). Unset flags cannot
   have any attached data. All unknown flags MUST be unset and their
   corresponding data removed when a tag is modified.

   Every set flag's data starts with a length byte, which contains a
   value between 0 and 128 ($00 - $7f), followed by data that has the
   field length indicated by the length byte. If a flag has no attached
   data, the value $00 is used as length byte.


   b - Tag is an update

     If this flag is set, the present tag is an update of a tag found
     earlier in the present file or stream. If frames defined as unique
     are found in the present tag, they are to override any
     corresponding ones found in the earlier tag. This flag has no
     corresponding data.

         Flag data length      $00

   c - CRC data present

     If this flag is set, a CRC-32 [ISO-3309] data is included in the
     extended header. The CRC is calculated on all the data between the
     header and footer as indicated by the header's tag length field,
     minus the extended header. Note that this includes the padding (if
     there is any), but excludes the footer. The CRC-32 is stored as an
     35 bit synchsafe integer, leaving the upper four bits always
     zeroed.

        Flag data length       $05
        Total frame CRC    5 * %0xxxxxxx

   d - Tag restrictions

     For some applications it might be desired to restrict a tag in more
     ways than imposed by the ID3v2 specification. Note that the
     presence of these restrictions does not affect how the tag is
     decoded, merely how it was restricted before encoding. If this flag
     is set the tag is restricted as follows:

        Flag data length       $01
        Restrictions           %ppqrrstt

     p - Tag size restrictions

       00   No more than 128 frames and 1 MB total tag size.
       01   No more than 64 frames and 128 KB total tag size.
       10   No more than 32 frames and 40 KB total tag size.
       11   No more than 32 frames and 4 KB total tag size.

     q - Text encoding restrictions

       0    No restrictions
       1    Strings are only encoded with ISO-8859-1 [ISO-8859-1] or
            UTF-8 [UTF-8].

     r - Text fields size restrictions

       00   No restrictions
       01   No string is longer than 1024 characters.
       10   No string is longer than 128 characters.
       11   No string is longer than 30 characters.

       Note that nothing is said about how many bytes is used to
       represent those characters, since it is encoding dependent. If a
       text frame consists of more than one string, the sum of the
       strungs is restricted as stated.

     s - Image encoding restrictions

       0   No restrictions
       1   Images are encoded only with PNG [PNG] or JPEG [JFIF].

     t - Image size restrictions

       00  No restrictions
       01  All images are 256x256 pixels or smaller.
       10  All images are 64x64 pixels or smaller.
       11  All images are exactly 64x64 pixels, unless required
           otherwise.

 */

// TODO - frame extended header flags


/*
   ID3v2 frame overview

   All ID3v2 frames consists of one frame header followed by one or more
   fields containing the actual information. The header is always 10
   bytes and laid out as follows:

     Frame ID      $xx xx xx xx  (four characters)
     Size      4 * %0xxxxxxx
     Flags         $xx xx

   The frame ID is made out of the characters capital A-Z and 0-9.
   Identifiers beginning with "X", "Y" and "Z" are for experimental
   frames and free for everyone to use, without the need to set the
   experimental bit in the tag header. Bear in mind that someone else
   might have used the same identifier as you. All other identifiers are
   either used or reserved for future use.

   The frame ID is followed by a size descriptor containing the size of
   the data in the final frame, after encryption, compression and
   unsynchronisation. The size is excluding the frame header ('total
   frame size' - 10 bytes) and stored as a 32 bit synchsafe integer.

   In the frame header the size descriptor is followed by two flag
   bytes. These flags are described in section 4.1.

   There is no fixed order of the frames' appearance in the tag,
   although it is desired that the frames are arranged in order of
   significance concerning the recognition of the file. An example of
   such order: UFID, TIT2, MCDI, TRCK ...

   A tag MUST contain at least one frame. A frame must be at least 1
   byte big, excluding the header.

   If nothing else is said, strings, including numeric strings and URLs
   [URL], are represented as ISO-8859-1 [ISO-8859-1] characters in the
   range $20 - $FF. Such strings are represented in frame descriptions
   as <text string>, or <full text string> if newlines are allowed. If
   nothing else is said newline character is forbidden. In ISO-8859-1 a
   newline is represented, when allowed, with $0A only.

   Frames that allow different types of text encoding contains a text
   encoding description byte. Possible encodings:

     $00   ISO-8859-1 [ISO-8859-1]. Terminated with $00.
     $01   UTF-16 [UTF-16] encoded Unicode [UNICODE] with BOM. All
           strings in the same frame SHALL have the same byteorder.
           Terminated with $00 00.
     $02   UTF-16BE [UTF-16] encoded Unicode [UNICODE] without BOM.
           Terminated with $00 00.
     $03   UTF-8 [UTF-8] encoded Unicode [UNICODE]. Terminated with $00.

   Strings dependent on encoding are represented in frame descriptions
   as <text string according to encoding>, or <full text string
   according to encoding> if newlines are allowed. Any empty strings of
   type $01 which are NULL-terminated may have the Unicode BOM followed
   by a Unicode NULL ($FF FE 00 00 or $FE FF 00 00).

   The timestamp fields are based on a subset of ISO 8601. When being as
   precise as possible the format of a time string is
   yyyy-MM-ddTHH:mm:ss (year, "-", month, "-", day, "T", hour (out of
   24), ":", minutes, ":", seconds), but the precision may be reduced by
   removing as many time indicators as wanted. Hence valid timestamps
   are
   yyyy, yyyy-MM, yyyy-MM-dd, yyyy-MM-ddTHH, yyyy-MM-ddTHH:mm and
   yyyy-MM-ddTHH:mm:ss. All time stamps are UTC. For durations, use
   the slash character as described in 8601, and for multiple non-
   contiguous dates, use multiple strings, if allowed by the frame
   definition.

   The three byte language field, present in several frames, is used to
   describe the language of the frame's content, according to ISO-639-2
   [ISO-639-2]. The language should be represented in lower case. If the
   language is not known the string "XXX" should be used.

   All URLs [URL] MAY be relative, e.g. "picture.png", "../doc.txt".

   If a frame is longer than it should be, e.g. having more fields than
   specified in this document, that indicates that additions to the
   frame have been made in a later version of the ID3v2 standard. This
   is reflected by the revision number in the header of the tag.
 */
typedef struct id3v2_frame_header_s {
	unsigned char id[5];
	uint32_t size;
	uint16_t flags;
} id3v2_frame_header_t;

/*
   Frame header flags

   In the frame header the size descriptor is followed by two flag
   bytes. All unused flags MUST be cleared. The first byte is for
   'status messages' and the second byte is a format description. If an
   unknown flag is set in the first byte the frame MUST NOT be changed
   without that bit cleared. If an unknown flag is set in the second
   byte the frame is likely to not be readable. Some flags in the second
   byte indicates that extra information is added to the header. These
   fields of extra information is ordered as the flags that indicates
   them. The flags field is defined as follows (l and o left out because
   ther resemblence to one and zero):

     %0abc0000 %0h00kmnp

   Some frame format flags indicate that additional information fields
   are added to the frame. This information is added after the frame
   header and before the frame data in the same order as the flags that
   indicates them. I.e. the four bytes of decompressed size will precede
   the encryption method byte. These additions affects the 'frame size'
   field, but are not subject to encryption or compression.

   The default status flags setting for a frame is, unless stated
   otherwise, 'preserved if tag is altered' and 'preserved if file is
   altered', i.e. %00000000.
 */


/*
   a - Tag alter preservation

     This flag tells the tag parser what to do with this frame if it is
     unknown and the tag is altered in any way. This applies to all
     kinds of alterations, including adding more padding and reordering
     the frames.

     0     Frame should be preserved.
     1     Frame should be discarded.
 */
#define FLAG_FR_TAG 0x4000

/*
   b - File alter preservation

     This flag tells the tag parser what to do with this frame if it is
     unknown and the file, excluding the tag, is altered. This does not
     apply when the audio is completely replaced with other audio data.

     0     Frame should be preserved.
     1     Frame should be discarded.
 */
#define FLAG_FR_FILE 0x2000

/*
   c - Read only

      This flag, if set, tells the software that the contents of this
      frame are intended to be read only. Changing the contents might
      break something, e.g. a signature. If the contents are changed,
      without knowledge of why the frame was flagged read only and
      without taking the proper means to compensate, e.g. recalculating
      the signature, the bit MUST be cleared.
 */
#define FLAG_FR_READ 0x1000

/*
   h - Grouping identity

      This flag indicates whether or not this frame belongs in a group
      with other frames. If set, a group identifier byte is added to the
      frame. Every frame with the same group identifier belongs to the
      same group.

      0     Frame does not contain group information
      1     Frame contains group information

 */
#define FLAG_FR_GROUP 0x0040

/*
   k - Compression

      This flag indicates whether or not the frame is compressed.
      A 'Data Length Indicator' byte MUST be included in the frame.

      0     Frame is not compressed.
      1     Frame is compressed using zlib [zlib] deflate method.
            If set, this requires the 'Data Length Indicator' bit
            to be set as well.
 */
#define FLAG_FR_COMP 0x0008

/*
   m - Encryption

      This flag indicates whether or not the frame is encrypted. If set,
      one byte indicating with which method it was encrypted will be
      added to the frame. See description of the ENCR frame for more
      information about encryption method registration. Encryption
      should be done after compression. Whether or not setting this flag
      requires the presence of a 'Data Length Indicator' depends on the
      specific algorithm used.

      0     Frame is not encrypted.
      1     Frame is encrypted.
 */
#define FLAG_FR_ENCR 0x0004

/*
   n - Unsynchronisation

      This flag indicates whether or not unsynchronisation was applied
      to this frame. See section 6 for details on unsynchronisation.
      If this flag is set all data from the end of this header to the
      end of this frame has been unsynchronised. Although desirable, the
      presence of a 'Data Length Indicator' is not made mandatory by
      unsynchronisation.

      0     Frame has not been unsynchronised.
      1     Frame has been unsyrchronised.
 */
#define FLAG_FR_UNSYNC 0x0002

/*
   p - Data length indicator

      This flag indicates that a data length indicator has been added to
      the frame. The data length indicator is the value one would write
      as the 'Frame length' if all of the frame format flags were
      zeroed, represented as a 32 bit synchsafe integer.

      0      There is no Data Length Indicator.
      1      A data length Indicator has been added to the frame.
 */
#define FLAG_FR_LEN 0x0001


int read_file(char * name, unsigned char ** p_buffer, unsigned long * p_len);

int parse_buffer(unsigned char *buffer, unsigned long buffer_len);

int parse_id3v2_header(unsigned char **p_header_buff, id3v2_header_t* header);

int parse_id3v2_frame_header(unsigned char **p_header_buff, id3v2_frame_header_t* header);

int parse_id3v2_frame_body(unsigned char **p_header_buff, id3v2_frame_header_t header);


void print_hexa(unsigned char *buffer, size_t len);

void print_id3v2_header(id3v2_header_t header);

void print_id3v2_frame_header(id3v2_frame_header_t header);


#endif /* ID3V2PARSER_H_ */
