#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <err.h>
#include <sysexits.h>
#include <stdio.h>
#include <string.h>

/* type of sizes and offsets */
typedef uint64_t myfs_size_t;

/* misc constants */
enum {
        /* myfs identifier */
        MYFS_MAGIC              = 0xbeef,

        /* inode type regular file */
        MYFS_INO_REG            = 0,
        /* inode type directory */
        MYFS_INO_DIR,
};

/* on disk version of super block */
struct myfs_super_block_ondisk {
        /* myfs identifier */
        myfs_size_t     sbo_magic;
        /* size of super block */
        myfs_size_t     sbo_sb_size;
        /* number of inodes */
        myfs_size_t     sbo_nr_ino;
        /* number of data blocks */
        myfs_size_t     sbo_nr_dat;
        /* number of data pointers per inode */
        myfs_size_t     sbo_ptr_per_ino;
        /* block size */
        myfs_size_t     sbo_blk_size;
        /* inode size */
        myfs_size_t     sbo_ino_size;
        /* data block size */
        myfs_size_t     sbo_dat_size;
        /* max length of directory entry name */
        myfs_size_t     sbo_dname_len;
};

/* in memory version of super block */
struct myfs_super_block {
        /* on disk version of super block */
        struct myfs_super_block_ondisk  sb_ondisk;
        /* file descriptor */
        int                             sb_fd;
};

/* every file has an associated inode */
struct myfs_inode {
        /* type of inode (regular file or directory) */
        myfs_size_t     i_type;
        /* size of file */
        myfs_size_t     i_size;
};

int
main(void)
{
        struct myfs_super_block sblk = {
                .sb_ondisk = {
                        .sbo_magic = MYFS_MAGIC,
                        .sbo_sb_size = sizeof(struct myfs_super_block_ondisk),
                        .sbo_nr_ino = 4,
                        .sbo_nr_dat = 8,
                        .sbo_ptr_per_ino = 2,
                        .sbo_blk_size = 512,
                        .sbo_ino_size = sizeof(struct myfs_inode) + (sizeof(myfs_size_t) * 2),
                        .sbo_dat_size = 128,
                        .sbo_dname_len = 16,
                },
        };
        struct myfs_super_block_ondisk *on = &sblk.sb_ondisk;
        size_t inotab_size;
        size_t inosize;
        ssize_t n;
        size_t dattab_size;
        int fd;
        char *buf;

        /* open file to write file system to */
        fd = open("myfs", O_CREAT | O_RDWR, 0666);
        if (fd < 0)
                err(EX_OSERR, "open");

        /* write super block with padding to get to block size */
        n = on->sbo_blk_size;
        printf("writing super block of size %ld bytes\n", n);
        buf = calloc(n, sizeof(*buf));
        if (buf == NULL)
                err(EX_SOFTWARE, "calloc failure while writing super block");
        memcpy(buf, on, sizeof(*on));
        if (write(fd, buf, n) != n)
                err(EX_OSERR, "write failure while writing super block");
        free(buf);
        buf = NULL;

        /* write inode table (and pad last block of inode table) */
        n = on->sbo_nr_ino;
        inosize = sizeof(struct myfs_inode) + (sizeof(myfs_size_t) * on->sbo_ptr_per_ino);
        inotab_size = inosize * n;
        if ((inotab_size % on->sbo_blk_size) != 0)
                inotab_size += on->sbo_blk_size - (inotab_size % on->sbo_blk_size);
        printf("writing inode table of size %ld bytes\n", inotab_size);
        buf = calloc(inotab_size, sizeof(*buf));
        if (buf == NULL)
                err(EX_OSERR, "calloc failure while writing inode table");
        if (write(fd, buf, inotab_size) != (ssize_t)inotab_size)
                err(EX_OSERR, "write failure while writing inode table");
        free(buf);
        buf = NULL;

        /* write data table (and pad last block of data table) */
        n = on->sbo_nr_dat;
        dattab_size = on->sbo_dat_size * n;
        if ((dattab_size % on->sbo_blk_size) != 0)
                dattab_size += on->sbo_blk_size - (dattab_size % on->sbo_blk_size);
        printf("writing data table of size %ld bytes\n", dattab_size);
        buf = calloc(dattab_size, sizeof(*buf));
        if (buf == NULL)
                err(EX_OSERR, "calloc failure while writing data table");
        if (write(fd, buf, dattab_size) != (ssize_t)dattab_size)
                err(EX_OSERR, "write failure while writing inode table");
        free(buf);
        buf = NULL;

        if (close(fd))
                err(EX_OSERR, "close");
}
