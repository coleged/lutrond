/*************************************************************************\
 *                  Copyright (C) Ed Cole 2018.                            *
 *                       colege@gmail.com                                  *
 *                                                                         *
 * This program is free software. You may use, modify, and redistribute it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation, either version 3 or (at your option) any      *
 * later version. This program is distributed without any warranty.  See   *
 * the file COPYING.gpl-v3 for details.                                    *
 *                                                                         *
 \*************************************************************************/

/**********
 
 readline.cpp
 
 lutrond V4.0 April 2018
 
 Readline() my_read() readline() readlinebuf()
 
 ***********/


#include    "lutrond.h"

static int    read_cnt;
static char    *read_ptr;
static char    read_buf[MAXLINE];

//******** my_read()
static ssize_t
my_read(int fd, char *ptr)
{
    
    if (read_cnt <= 0) {
    again:
        if ( (read_cnt = (int)read(fd, read_buf, (int)sizeof(read_buf))) < 0) {
            if (errno == EINTR)
                goto again;
            return(-1);
        } else if (read_cnt == 0)
            return(0);
        read_ptr = read_buf;
    }
    
    read_cnt--;
    *ptr = *read_ptr++;
    return(1);
}

//******** readline()
ssize_t
readline(int fd, void *vptr, size_t maxlen)
{
    ssize_t    n, rc;
    char    c, *ptr;
    
    ptr = (char *)vptr;
    for (n = 1; n < maxlen; n++) {
        if ( (rc = my_read(fd, &c)) == 1) {
            *ptr++ = c;
            if (c == '\n')
                break;    /* newline is stored, like fgets() */
        } else if (rc == 0) {
            *ptr = 0;
            return(n - 1);    /* EOF, n - 1 bytes were read */
        } else
            return(-1);        /* error, errno set by read() */
    }
    
    *ptr = 0;    /* null terminate like fgets() */
    return(n);
}

//******** readlinebuf()
ssize_t
readlinebuf(void **vptrptr)
{
    if (read_cnt)
        *vptrptr = read_ptr;
    return(read_cnt);
}
/* end readline */

//******** Readline()
ssize_t
Readline(int fd, void *ptr, size_t maxlen)
{
    ssize_t        n;
    
    if ( (n = readline(fd, ptr, maxlen)) < 0)
        printf("readline error\n");   // TODO fix err_sys linkage
        //err_sys("readline error");
    return(n);
}

