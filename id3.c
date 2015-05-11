#include "id3.h"
#include <stdarg.h>
#include <wchar.h>

// Map the song to an array
unsigned char *file;

header_t *getHeader();
frame_t *getNextFrame();
frame_t *saveAlbumArt(frame_t *frame);

char *songFilename;

// Inreases the position pointer by n
void increasePos(int n);
int currentPosition = 0;

void printHeader(header_t *header);
void printFrame(frame_t *frame);
void errExit(char *errMsg, ...);


// A table of frame pointers
// Each index points to the frame defined in the enum

const char *frameStrings[] = {
    "AENC", "APIC", "COMM", "COMR", "ENCR", "EQUA", "ETCO", "GEOB", "GRID", "IPLS", "LINK", "MCDI", 
    "MLLT", "OWNE", "PRIV", "PCNT", "POPM", "POSS", "RBUF", "RVAD", "RVRB", "SYLT", "SYTC", "TALB",
    "TBPM", "TCOM", "TCON", "TCOP", "TDAT", "TDLY", "TENC", "TEXT", "TFLT", "TIME", "TIT1", "TIT2",
    "TIT3", "TKEY", "TLAN", "TLEN", "TMED", "TOAL", "TOFN", "TOLY", "TOPE", "TORY", "TOWN", "TPE1",
    "TPE2", "TPE3", "TPE4", "TPOS", "TPUB", "TRCK", "TRDA", "TRSN", "TRSO", "TSIZ", "TSRC", "TSSE",
    "TYER", "TXXX", "UFID", "USER", "USLT", "WCOM", "WCOP", "WOAF", "WOAR", "WOAS", "WORS", "WPAY",
    "WPUB", "WXXX"
};

typedef enum {
    AENC, APIC, COMM, COMR, ENCR, EQUA, ETCO, GEOB, GRID, IPLS, LINK, MCDI, 
    MLLT, OWNE, PRIV, PCNT, POPM, POSS, RBUF, RVAD, RVRB, SYLT, SYTC, TALB,
    TBPM, TCOM, TCON, TCOP, TDAT, TDLY, TENC, TEXT, TFLT, TIME, TIT1, TIT2,
    TIT3, TKEY, TLAN, TLEN, TMED, TOAL, TOFN, TOLY, TOPE, TORY, TOWN, TPE1,
    TPE2, TPE3, TPE4, TPOS, TPUB, TRCK, TRDA, TRSN, TRSO, TSIZ, TSRC, TSSE,
    TYER, TXXX, UFID, USER, USLT, WCOM, WCOP, WOAF, WOAR, WOAS, WORS, WPAY,
    WPUB, WXXX
} frameIds;


char *songFilename; 

int main(int argc, char *argv[])
{
    // Verify Arguments
    if (argc < 2) {
        printf("Usage: id3Reader <mp3 songFilename>\n");
        exit(1);
    }

    songFilename = argv[1];

    int fd = open(songFilename, O_RDONLY);
    struct stat st;
    fstat(fd, &st); 

    song_t *songs[1];
    songs[0] = malloc(sizeof(song_t));

    // Couldn't open the file
    if (fd == -1) {
        if (errno == EACCES) {
            errExit("Do not have access to open %s.", songFilename);
        }
        else if (errno == ENOENT) {

            errExit("The file %s does not exist.", songFilename);
        }
        else {  
            errExit("Could not open the file");
        }
    }

    // Map the mp3 file
    file = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (file == MAP_FAILED) {
        errExit("Could not map file.");
    }

    // Get the id3 header
    header_t *header = getHeader();

    // Check major version
    if (header->version != 3) {
        printf("id3Read currently only supports id3v2.3 this file is verion id3v2.%d.\n", header->version);
        exit(1);
    }

    // Go through all of the frames in the document 
    for (;;) {
        frame_t *frame = getNextFrame();
        if (strcmp(frame->header->id, "") != 0) {
             
            printFrame(frame);
        }
        if (currentPosition + sizeof(frame_t) > header->size) {
            break;
        }
        free(frame);
    }
}

void printHeader(header_t *header) {
    printf("Header id: %s version: %d revision: %d unsynchronized: %d extended: %d experimental: %d size: %d\n",
            header->id, header->version, header->revision, header->unsynchronized, 
            header->extended, header->experimental, header->size);

}

void printFrame(frame_t *frame) {
    frameHeader_t *header = frame->header;
    printf("Frame Header id: %s Size: %d tagPreserve: %d filePreserver: %d readOnly: %d compressed: %d encryption: %d group %d\n", header->id, header->size, 
            header->tagPreservation, header->filePreservation, header->readOnly, header->compressed, header->encryption, header->group);
    printf("Frame Contents %s\n", frame->attribute);
}

// Current Position is the current position in the file
void increasePos(int n) {
    currentPosition += n;    
}

void printUsage() {
    DieWithError("Usage: %s mp3File");
}

void DieWithError(char *errMsg) {
    fprintf(stderr, "Error: %s\n", errMsg);
    exit(1);
}

// Memcpy wrapper that increments the position pointer
void copyTo(void *dst, size_t size) {
    memcpy(dst, file + currentPosition, size);
    increasePos(size);  
}


header_t *getHeader() {
    rawHeader_t *rawHeader = malloc(sizeof(rawHeader_t));
    header_t *header = malloc(sizeof(header_t));
    // Copies the header data into the header struct
    copyTo(rawHeader, sizeof(rawHeader_t));

    memcpy(header->id, rawHeader->id, sizeof(char) * 3);
    header->id[3] = '\0'; 

    if (strcmp(header->id, "ID3") != 0) {
        DieWithError("Not an ID3 tag!");    
    }

    // Get the version/revision data
    header->version = rawHeader->version;
    header->revision = rawHeader->revision;

    // Check for unsychronized, extended, and experimental flags
    header->unsynchronized = ((rawHeader->flags & 0x80) >> 7);
    header->extended = ((rawHeader->flags & 0x40) >> 6);
    header->experimental = ((rawHeader->flags & 0x20) >> 5);

    // if any other flag bits are set, it's an error
    if (rawHeader->flags & 0x1F) {
        DieWithError("Invalid flag bits are set!");
    }

    // Get the size. No idea why they chose this 
    // Basically they store the size in 4 bytes but you only actually count the last 3 bits of every nibble
    // Soo it's 28 bits stored in 32 bits of space
    // It's mystifying really
    unsigned int size = 0;

    for (int i = 0; i < 4; i++) {
        // printf("%x ", rawHeader->size[i]);
        unsigned int value = (rawHeader->size[i] & 0x7F); 
        value <<= (21 - i * 7);
        size |= value;
    }
    header->size = size;

    free(rawHeader);
    return header;
}

frame_t *getNextFrame() {

    // TODO Need to figure out how to handle the different types of TRCK entries
    rawFrameHeader_t *rawHeader = malloc(sizeof(rawFrameHeader_t));
    frameHeader_t *header = malloc(sizeof(frameHeader_t));
    frame_t *frame = malloc(sizeof(frame));

    // Copy the data from the song to the raw frame header
    copyTo(rawHeader, sizeof(rawFrameHeader_t));

    memcpy(header->id, rawHeader->id, sizeof(char) * 4);
    header->id[4] = '\0'; 

    // Get the size
    header->size = htonl(*((unsigned int *) rawHeader->size));    

    // Check for unsychronized, extended, and experimental flags
    header->tagPreservation = ((rawHeader->flags[0] & 0x80) >> 7);
    // Check to see if the file should be preserved 
    header->filePreservation = ((rawHeader->flags[0] & 0x40) >> 6);;
    // Is the file read only?
    header->readOnly =  ((rawHeader->flags[0] & 0x20) >> 5);
    // Is the frame compressed?
    header->compressed = ((rawHeader->flags[1] & 0x80) >> 7);
    // Encryption
    header->encryption = ((rawHeader->flags[1] & 0x40) >> 6);
    // Is the frame part of a group
    header->group = ((rawHeader->flags[1] & 20) >> 5);

    frame->header = header;
    free(rawHeader);
    // Get the attribute within the frame
    //frame->attribute = malloc(header->size + 1);
    //copyTo(frame->attribute, header->size);

    // Save the album art to disk
    if (strcmp(header->id, "APIC") == 0) 
      saveAlbumArt(frame);

    unsigned char *attribute = malloc(header->size + 1);
    copyTo(attribute, header->size);
    // Skip leading  0's
    for (int i = 0; attribute[i] == 0 &&  i < header->size; i++) {
        attribute++;
    }

    // Attribute is in utf_16 Big Endian with the start of header prepended
    if (attribute[0] == 0x1 && attribute[1] == 0xFF && attribute[2] == 0xFE) {
        attribute += 3;        
        int newSize = (header->size - 3)/2;
        char *newAttribute = malloc(newSize + 1);
        for (int i = 0; i < header->size - 3; i++) {
            // Copy odd characters to the string
            if (i % 2 == 0) {
                newAttribute[i/2] = attribute[i];
            }
        }
        for (int i = 0; i < newSize; i++) {
        }
        newAttribute[newSize] = '\0';
        free(attribute - 3);
        frame->attribute = newAttribute;
    }
    else if (attribute[1] == 0xFF && attribute[2] == 0xFE) {
        // Should do the same thing as the previous clause
    }
    else {
        attribute[header->size] = '\0';
        frame->attribute = (char *)attribute;
    }
    return frame;
}

frame_t *saveAlbumArt(frame_t *frame) { 
        frameHeader_t *header = frame->header;
        int imageHeaderLen = 0;

        // States which character encoding is used
        char encoding;
        copyTo(&encoding, 1);
        imageHeaderLen++;
        if (encoding != 0) {
            // Not sure what to do with unicode here yet
            exit(1);
        }

        // Have to get the length of the string first
        int mimeLen = strlen((char *) file + currentPosition);
        //printf("Pos %d\n", currentPosition);
        //printf("mimeLen: %d\n", mimeLen);
        char *mimeType = malloc(mimeLen + 1);
        // Add 1 to copy the null byte as well
        copyTo(mimeType, mimeLen + 1);
        imageHeaderLen += mimeLen + 1;

        char pictureType;
        copyTo(&pictureType, 1);
        imageHeaderLen++;
        // Get the description
        int descriptionLen = strlen((char *) file + currentPosition);
        char *description = malloc(descriptionLen + 1);
        copyTo(description, descriptionLen + 1);
        if (strcmp(description, "") == 0) 
            description = "None";
        imageHeaderLen += descriptionLen + 1;

        //printf("Encoding: %x mimeType: %s pictureType: %x description: %s \n", encoding, mimeType, pictureType, description);
        // The size of the image is the size given in the frame header minus the size of the image header
        int imageSize = header->size - imageHeaderLen;
        unsigned char *imageData = malloc(header->size - imageHeaderLen);
        copyTo(imageData, imageSize);

        // Figure out the extension
        char *extension = NULL;
        if (strstr(mimeType, "jpeg") != NULL || strstr("mimeType", "jpeg") != NULL) {
            extension = ".jpg";
        }
        else if (strstr(mimeType, "bmp") != NULL) {
            extension = ".bmp";
        }
        else if (strstr(mimeType, "png") != NULL) {
            extension = ".png";
        }
        else {
            puts("Could not get extension");
            return NULL;
        }


        char *extLoc = strstr(songFilename, ".mp3");
        // Don't need the 3 bytes from the mp3 
        char *imageFilename = malloc(strlen(songFilename) - strlen(".mp3") + strlen(extension) + 1);
        // The name of the file without the mp3 tag
        int filenameLen = strlen(songFilename) - 4 + 1;
        char *filename = malloc(filenameLen);
        memcpy(filename, songFilename, extLoc - songFilename);
        filename[filenameLen] = '\0';

        sprintf(imageFilename, "%s%s", filename, extension);
        printf("Writing %s\n", imageFilename);

        FILE *image = fopen(imageFilename, "wb");

        fwrite(imageData, 1, imageSize, image);
        frame->attribute = "image";
        return frame;
    }

void errExit(char *errMsg, ...) {
    va_list args;
    va_start(args, errMsg);
    vfprintf(stderr, errMsg, args);
    fprintf(stderr, "\n");
    exit(1);
}
