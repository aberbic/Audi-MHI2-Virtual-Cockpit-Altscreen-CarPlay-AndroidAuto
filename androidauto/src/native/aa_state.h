#ifndef AA_STATE_H
#define AA_STATE_H
#include <fcntl.h>
#include <unistd.h>
/* /tmp is QNX /dev/shmem, not a full filesystem. No rename or directory
 * operations: overwrite one byte at offset zero, without truncation. */
static inline int aa_state_write(const char *path,int ready)
{
    char value=ready?'1':'0';ssize_t n;int fd=open(path,O_WRONLY|O_CREAT,0600);
    if(fd<0)return -1;
    n=write(fd,&value,1);close(fd);return n==1?0:-1;
}
static inline int aa_state_read(const char *path)
{
    char value=0;ssize_t n;int fd=open(path,O_RDONLY);
    if(fd<0)return 0;
    n=read(fd,&value,1);close(fd);return n==1&&value=='1';
}
#endif
