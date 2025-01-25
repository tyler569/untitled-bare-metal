#include "stdint.h"
#include "sys/cdefs.h"

struct PACKED tar_header
{
  char name[100];     // File name
  char mode[8];       // File mode
  char uid[8];        // Owner's numeric user ID
  char gid[8];        // Group's numeric user ID
  char size[12];      // File size in bytes
  char mtime[12];     // Time of last modification in numeric Unix time format
  char chksum[8];     // Checksum for header record
  char typeflag;      // Type of file
  char linkname[100]; // Target name for link
  char magic[6];      // UStar indicator "ustar"
  char version[2];    // UStar version "00"
  char uname[32];     // Owner user name
  char gname[32];     // Owner group name
  char devmajor[8];   // Major device number (if typeflag is 'b' or 'c')
  char devminor[8];   // Minor device number (if typeflag is 'b' or 'c')
  char prefix[155];   // Prefix for file name (if file name is too long)
};

#define TMAGIC "ustar" /* ustar and a null */
#define TMAGLEN 6
#define TVERSION "00" /* 00 and no null */
#define TVERSLEN 2

#define REGTYPE '0'   /* regular file */
#define AREGTYPE '\0' /* regular file */
#define LNKTYPE '1'   /* link */
#define SYMTYPE '2'   /* reserved */
#define CHRTYPE '3'   /* character special */
#define BLKTYPE '4'   /* block special */
#define DIRTYPE '5'   /* directory */
#define FIFOTYPE '6'  /* FIFO special */
#define CONTTYPE '7'  /* reserved */

void *find_tar_entry (struct tar_header *tar, const char *name);
