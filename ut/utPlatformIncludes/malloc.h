#ifndef _INCLUDE_MALLOC_H_
#define _INCLUDE_MALLOC_H_

struct mallinfo {
    int arena;    /* size of the arena */
    int ordblks;  /* number of big blocks in use */
    int smblks;   /* number of small blocks in use */
    int hblks;    /* number of header blocks in use */
    int hblkhd;   /* space in header block headers */
    int usmblks;  /* space in small blocks in use */
    int fsmblks;  /* memory in free small blocks */
    int uordblks; /* space in big blocks in use */
    int fordblks; /* memory in free big blocks */
    int keepcost; /* penalty if M_KEEP is used
                                -- not used */
};

static inline struct mallinfo mallinfo()
{
    struct mallinfo x = {};
    return x;
}

#endif
