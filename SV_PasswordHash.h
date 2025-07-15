#define byteSwap32(x) (((x) >> 24) | (((x)&0x00FF0000) >> 8) | (((x)&0x0000FF00) << 8) | ((x) << 24))
#define byteSwap64(x)                                                      \
        ((((x) >> 56) & 0x00000000000000FF) | (((x) >> 40) & 0x000000000000FF00) | \
         (((x) >> 24) & 0x0000000000FF0000) | (((x) >> 8) & 0x00000000FF000000) |  \
         (((x) << 8) & 0x000000FF00000000) | (((x) << 24) & 0x0000FF0000000000) |  \
         (((x) << 40) & 0x00FF000000000000) | (((x) << 56) & 0xFF00000000000000))

// Define a union for easy reference
// Union represents a message block
union messageBlock
{
    __uint8_t e[64];
    __uint32_t t[16];
    __uint64_t s[8];
};

// ENUM to control state of the program
enum status{READ,
            PAD0,
            PAD1,
            FINISH
            };

// Tell our preprocessor to create a variable MAXCHAR with value of 100000
#define MAXCHAR 100000

// Function decleration
// See Section 4.1.2
__uint32_t sig0(__uint32_t x);
__uint32_t sig1(__uint32_t x);

__uint32_t rotr(__uint32_t n, __uint16_t x);
__uint32_t shr(__uint32_t n, __uint16_t x);

__uint32_t SIG0(__uint32_t x);
__uint32_t SIG1(__uint32_t x);

__uint32_t Ch(__uint32_t x,__uint32_t y,__uint32_t z);
__uint32_t Maj(__uint32_t x,__uint32_t y,__uint32_t z);

_Bool endianCheck(void);
void endianCheckPrint(void);
int calcFileSize(FILE *);
void printFileContents(FILE *);
int fillMessageBlock(FILE *, union messageBlock *, enum status *, __uint64_t *);
void calculateHash(FILE *);
char *passwordHash(int, char *);
