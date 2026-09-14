/////////////////////////////////////////////////////////////////////////////
//
// filecache - code for background file reading/caching
//
/////////////////////////////////////////////////////////////////////////////

#ifndef FILECACHE_H
#define FILECACHE_H

/////////////////////////////////////////////////////////////////////////////
//
// BLOCKS MUST BE 255 OR LESS
//
// hint: if you set realfd to a negative number, that means it'll use the
// gdrom routines to read a pak file starting at that (positive) lba
//
void filecache_init(
    int realfd,
    unsigned int pakcdsectors,
    unsigned int blocksize,
    unsigned char blocks,
    unsigned int vfds
);

//
// Clear All Allocations
//
void filecache_term(void);

//
// attempt to read a block
// returns the number of bytes read or 0 on error
//
int filecache_readpakblock(
    unsigned char *dest,
    unsigned int pakblock,
    int startofs,
    int bytes,
    int blocking
);

//
// set up where the vfd pointers are
//
void filecache_setvfd(unsigned int vfd, unsigned int start, unsigned int block, unsigned int readahead);

//
// call this every now and then
//
void filecache_process(void);

void filecache_wait_for_prebuffer(unsigned int vfd, unsigned int nblocks);

/////////////////////////////////////////////////////////////////////////////

#endif

