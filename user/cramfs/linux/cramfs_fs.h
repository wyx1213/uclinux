#ifndef __CRAMFS_FS_H
#define __CRAMFS_FS_H

#ifdef __KERNEL__

#include <asm/byteorder.h>

/* Uncompression interfaces to the underlying zlib */
int cramfs_uncompress_block(void *dst, int dstlen, void *src, int srclen);
int cramfs_uncompress_init(void);
int cramfs_uncompress_exit(void);

#else /* not __KERNEL__ */

#ifdef __linux__
#include <byteswap.h>
#endif
#ifndef bswap_16
# define bswap_16(x) ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8))
#endif
#ifndef bswap_32
# define bswap_32(x) \
     ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) | \
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))
#endif

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

#endif /* not __KERNEL__ */

#define CRAMFS_MAGIC		0x28cd3d45	/* some random number */
#define CRAMFS_SIGNATURE	"Compressed ROMFS"

/*
 * Width of various bitfields in struct cramfs_inode.
 * Primarily used to generate warnings in mkcramfs.
 */
#define CRAMFS_MODE_WIDTH 16
#define CRAMFS_UID_WIDTH 16
#define CRAMFS_SIZE_WIDTH 24
#define CRAMFS_GID_WIDTH 8
#define CRAMFS_NAMELEN_WIDTH 6
#define CRAMFS_OFFSET_WIDTH 26

/*
 * Since inode.namelen is a unsigned 6-bit number, the maximum cramfs
 * path length is 63 << 2 = 252.
 */
#define CRAMFS_MAXPATHLEN (((1 << CRAMFS_NAMELEN_WIDTH) - 1) << 2)

/*
 * Reasonably terse representation of the inode data.
 */
struct cramfs_inode {
	u32 mode:CRAMFS_MODE_WIDTH, uid:CRAMFS_UID_WIDTH;
	/* SIZE for device files is i_rdev */
	u32 size:CRAMFS_SIZE_WIDTH, gid:CRAMFS_GID_WIDTH;
	/* NAMELEN is the length of the file name, divided by 4 and
           rounded up.  (cramfs doesn't support hard links.) */
	/* OFFSET: For symlinks and non-empty regular files, this
	   contains the offset (divided by 4) of the file data in
	   compressed form (starting with an array of block pointers;
	   see README).  For non-empty directories it is the offset
	   (divided by 4) of the inode of the first file in that
	   directory.  For anything else, offset is zero. */
	u32 namelen:CRAMFS_NAMELEN_WIDTH, offset:CRAMFS_OFFSET_WIDTH;
};

struct cramfs_info {
	u32 crc;
	u32 edition;
	u32 blocks;
	u32 files;
};

/*
 * Superblock information at the beginning of the FS.
 */
struct cramfs_super {
	u32 magic;			/* 0x28cd3d45 - random number */
	u32 size;			/* length in bytes */
	u32 flags;			/* feature flags */
	u32 future;			/* reserved for future use */
	u8 signature[16];		/* "Compressed ROMFS" */
	struct cramfs_info fsid;	/* unique filesystem info */
	u8 name[16];			/* user-defined name */
	struct cramfs_inode root;	/* root inode data */
};

/*
 * Feature flags
 *
 * 0x00000000 - 0x000000ff: features that work for all past kernels
 * 0x00000100 - 0xffffffff: features that don't work for past kernels
 */
#define CRAMFS_FLAG_FSID_VERSION_2	0x00000001	/* fsid version #2 */
#define CRAMFS_FLAG_SORTED_DIRS		0x00000002	/* sorted dirs */
#define CRAMFS_FLAG_HOLES		0x00000100	/* support for holes */
#define CRAMFS_FLAG_WRONG_SIGNATURE	0x00000200	/* reserved */
#define CRAMFS_FLAG_SHIFTED_ROOT_OFFSET	0x00000400	/* shifted root fs */

/*
 * Valid values in super.flags.  Currently we refuse to mount
 * if (flags & ~CRAMFS_SUPPORTED_FLAGS).  Maybe that should be
 * changed to test super.future instead.
 */
#define CRAMFS_SUPPORTED_FLAGS	( 0x000000ff \
				| CRAMFS_FLAG_HOLES \
				| CRAMFS_FLAG_WRONG_SIGNATURE \
				| CRAMFS_FLAG_SHIFTED_ROOT_OFFSET )

/*
 * Since cramfs is little-endian, provide macros to swab the bitfields.
 */

#if defined(CRAMFS_LITTLE_ENDIAN)
#define CRAMFS_16(x)	(x)
#define CRAMFS_24(x)	(x)
#define CRAMFS_32(x)	(x)
#define CRAMFS_GET_NAMELEN(x)	((x)->namelen)
#define CRAMFS_GET_OFFSET(x)	((x)->offset)
#define CRAMFS_SET_OFFSET(x,y)	((x)->offset = (y))
#define CRAMFS_SET_NAMELEN(x,y)	((x)->namelen = (y))
#elif defined(CRAMFS_BIG_ENDIAN)
#ifdef __KERNEL__
#define CRAMFS_16(x)	swab16(x)
#define CRAMFS_24(x)	((swab32(x)) >> 8)
#define CRAMFS_32(x)	swab32(x)
#else /* not __KERNEL__ */
#define CRAMFS_16(x)	bswap_16(x)
#define CRAMFS_24(x)	((bswap_32(x)) >> 8)
#define CRAMFS_32(x)	bswap_32(x)
#endif /* not __KERNEL__ */
#define CRAMFS_GET_NAMELEN(x)	(((u8*)(x))[8] & 0x3f)
#define CRAMFS_GET_OFFSET(x)	((CRAMFS_24(((u32*)(x))[2] & 0xffffff) << 2) |\
				 ((((u32*)(x))[2] & 0xc0000000) >> 30))
#define CRAMFS_SET_NAMELEN(x,y)	(((u8*)(x))[8] = (((0x3f & (y))) | \
						  (0xc0 & ((u8*)(x))[8])))
#define CRAMFS_SET_OFFSET(x,y)	(((u32*)(x))[2] = (((y) & 3) << 30) | \
				 CRAMFS_24((((y) & 0x03ffffff) >> 2)) | \
				 (((u32)(((u8*)(x))[8] & 0x3f)) << 24))
#else
#error "__BYTE_ORDER must be __LITTLE_ENDIAN or __BIG_ENDIAN"
#endif

#endif /* not __CRAMFS_FS_H */
