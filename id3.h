#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>

typedef struct song {
    // TPE1 
    char *leadPerformer;
    // TALB
    char *album;
    // TIT2
    char *title;
    // TPUB
    char *publisher;
    // TRCK
    char *trackNumber;
    // TCOM
    char *composer;
    // TLEN
    char *songLength;
    // TENC
    char *enconding;
    

} song_t;

//#pragma pack(push)
//#pragma pack(1)
typedef struct rawHeader_t {
    unsigned char id[3];
    unsigned char version;
    unsigned char revision;
    unsigned char flags;
    unsigned char size[4];
} rawHeader_t;
//#pragma pack(pop)

#pragma pack(push)
#pragma pack(1)
typedef struct rawFrameHeader_t {
    unsigned char id[4];
    unsigned char size[4];
    unsigned char flags[2];
} rawFrameHeader_t;
#pragma pack(pop)

#pragma pack(push)
#pragma pack(1)
typedef struct header_t {
    // Should be id3
    char id[4];
    // Specifies the major version 
    short version;
    // Specifies the revision e.g. 3.1
    short revision;
    // Whether or not unsychronization is used
    bool unsynchronized;
    // Is there an extended header?
    bool extended;
    // Is it an experminetal tag?
    bool experimental;
    // The size of the id3 tag
    unsigned int size;
} header_t;
#pragma pack(pop)

#pragma pack(push)
#pragma pack(1)
typedef struct frameHeader_t {
    // The frame id. What kind of frame is it?
    char id[5];
    // The size of the frame
    unsigned int size;
    // Should the frame be preserver
    bool tagPreservation;
    // Should the file be preserved or discarded
    bool filePreservation;
    // Is the file read only?
    bool readOnly;
    // Is the frame compressed?
    bool compressed;
    // Encryption
    bool encryption;
    // Is the frame part of a group
    bool group;


} frameHeader_t;
#pragma pack(pop)

typedef struct frame_t {
    frameHeader_t *header;
    char *attribute;
} frame_t;

typedef struct extendedHeader {

} extendedHeader;

typedef struct tag {
    int version;
    bool extended;
    bool experimental;
} tag;

void DieWithError(char *errMsg);


