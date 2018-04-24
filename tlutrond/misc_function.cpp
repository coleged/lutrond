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
 
 misc_function.cpp
 
 lutrond V4.0 April 2018
 
 argstr() logClose() logMessage() logOpen() pidFile() strarg() testRoot()
 
 ***********/

#include "lutrond.h"
#include "externals.h"

//************   testRoot
int testRoot(){
    
     int user;
     
     user=getuid();
     if(user != 0){
     fprintf(stderr,"Must be root to run this\n");
     error("Not root!");
     }//if
    
    return(EXIT_SUCCESS);
    
}//testRoot

//************   logMessage
void
logMessage(const char *format, ...)
{
    va_list argList;
    const char *TIMESTAMP_FMT = "%F %X";   /* = YYYY-MM-DD HH:MM:SS */
    char timestamp[TS_BUF_SIZE];
    time_t t;
    struct tm *loc;
    
    t = time(NULL);
    loc = localtime(&t);
    if (  loc == NULL ||
        strftime(timestamp, TS_BUF_SIZE, TIMESTAMP_FMT, loc) == 0){
        fprintf(admin.logfp, "Unknown time");
    }else{
        fprintf(admin.logfp, "%s: ", timestamp);
    }//else
    va_start(argList, format);
    vfprintf(admin.logfp, format, argList);
    fprintf(admin.logfp, "\n");
    va_end(argList);
}//logMessage


//************   logOpen
void
logOpen(const char *logFilename)
{
    mode_t m;
    
    m = umask(033);
    admin.logfp = fopen(admin.log_file, "a");
    fcntl(fileno(admin.logfp), F_SETFD, FD_CLOEXEC);
    umask(m);
    
    if (admin.logfp == NULL){ // opening log file failed
        fprintf(stderr,"Unable to open log filei\n");
        exit(EXIT_FAILURE);
    }//if
    if(flag.debug) printf("log file open %s (FD=%i)",admin.log_file,fileno(admin.logfp));
    setbuf(admin.logfp, NULL);     /* Disable stdio buffering */
    if(flag.debug) logMessage("Opened log file");
    
}//logOpen

/************   logClose
// Not used
static void
logClose(void)
{
    if(flag.debug) logMessage("Closing log file");
    fclose(admin.logfp);
    
}//logClose */


//************   pidFile - write my (parent) pid to pid file
void
pidFile(const char *file,char *command_line)
{
    
    static FILE *pidfp;
    
    mode_t m;
    m = umask(033);
    pidfp = fopen(admin.pid_file, "w");
    umask(m);
    
    if (pidfp == NULL){
        logMessage("Unable to open PID file");
        exit(EXIT_FAILURE);
    }
    if(flag.debug) printf("pid file open (FD=%i)",fileno(pidfp));
    fprintf(pidfp,"%i\n",getpid());  // pid on line 1
    fprintf(pidfp,"%s\n",command_line); // command line on line 2
    fflush(pidfp);
    fclose(pidfp);
    
}//pidFile

//************   argstr
char *argstr(int count, char **arg_list) {
    // takes count and *arg_list[] and returns argv_list[] as single string with
    // each array element as space separated word in string
    // returns NULL pointer on error
    
    // Allocate memory for each arg sub-string, plus one inter argument,
    // space plus the NUL terminator.
    size_t size = 1;
    for (int i = 0; i < count; i++) {
        size += strlen(arg_list[i]) + 1; // total string length required
    }                     // tok1 tok2 .... tokN<NUL>
    char *str = (char *)malloc(size);
    if (!str) {
        fprintf(stderr, "No memory\n");
        return(NULL);
    }
    
    // Concatenate all arguments with a trailing space after each.
    // `size` keeps a running total of the length of the string being built.
    // sprintf() copies the argument to the buffer starting at the
    // offset specified as `size`.  There is also a space appended
    // (because it is specified by the format string) and a NUL
    // terminator (because that's what sprintf() does).
    // sprintf() returns the number of bytes written (excluding the
    // NUL that it added), which we use to keep track of the offset
    // of the NUL terminator.
    
    
    size = 0;
    for (int i = 0; i < count; i++) {
        size += sprintf(str + size, "%s ", arg_list[i]);
        
    }
    
    // Remove trailing space.  The string is now terminated by two
    // consecutive NULs.  That's OK: only the first one matters.
    
    str[--size] = '\0';
    
    return(str);
}


//************ strarg()

char **strarg(char *str){ // takes a string of whitespace separated tokens
    // and breaks out tokens into an array of pointers
    // to strings will last element a NULL pointer
    
    int len = (int)strlen(str);
    char *line;   // pointer to working buffer for strtok
    int n = 0;
    
    line = (char *)malloc(len * sizeof(char));    // create working buffer
    // create array of pointers to strings with
    // two element args[0] and argv[1]
    char** args = (char **)malloc( (n+2) * sizeof(char*));
    memcpy(line,str,len);          // copy string into working buffer
    args[n] = strtok(line,"\t ");  // break out first token
    ++n;
    
    while((args[n] = strtok(NULL,"\t ")) != NULL){
        // scan for more tokens growing pointer array
        ++n;
        args = (char **)realloc(args, (n+2) * sizeof(char*));
    }
    // args[n] = NULL per the while test, so we're all done
    
    return(args);
    
}






