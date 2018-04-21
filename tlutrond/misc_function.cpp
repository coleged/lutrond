//
//  misc_function.cpp
//  tlutrond
//
//  Created by Ed Cole on 20/04/2018.
//  Copyright © 2018 Ed Cole. All rights reserved.
//

#include "lutrond.h"
#include "externals.h"

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

//************   logClose
static void
logClose(void)
{
    if(flag.debug) logMessage("Closing log file");
    fclose(admin.logfp);
    
}//logClose

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

//************** timestamp()
char *timestamp(){
    
    static char stamp[14];   //yyyymmddhhmmss
    
    time_t now = time(NULL);
    struct tm *today = gmtime(&now);
    int year = today->tm_year + 1900;
    int month = today->tm_mon + 1;
    int day = today->tm_mday;
    int hour = today->tm_hour;
    if (today->tm_isdst == 0) ++hour;
    int min = today->tm_min;
    int sec = today->tm_sec;
    
    sprintf(stamp,"%i%02d%02d%02d%02d%02d",year,month,day,hour,min,sec);
    
    return(stamp);
    
}// timestamp
//*************** timestamp()

//*************** dump_db()
void
dump_db(){
    
    int i,j;
    char dbFilename[128];
    static FILE *dbfp;
    
    bzero(dbFilename,128);
    sprintf(dbFilename,"%s.%s",admin.db_file,timestamp());
    mode_t m;
    m = umask(033);
    dbfp = fopen(dbFilename, "w");
    umask(m);
    
    if (dbfp == NULL){
        logMessage("Unable to open database file");
    }
    
    for(i=1;i<NO_OF_DEVICES;i++){
        // TODO: xcode warns that second condition is always true. Check logic.
        if ((device[i].type != 0) && (device[i].state != NULL)){
            fprintf(dbfp,"device_%i = {\n",i);
            fprintf(dbfp,"\ttype = %i;\n",device[i].type);
            fprintf(dbfp,"\tlocation = \"%s\";\n",device[i].location);
            fprintf(dbfp,"\tname = \"%s\";\n",device[i].name);
            fprintf(dbfp,"\tstate = \"%s\";\n",device[i].state);
            for( j=1;j<NO_OF_COMPS;++j){
                if( device[i].comp[j].num > 0){
                    fprintf(dbfp,"\tcomp_%i = {\n",j);
                    fprintf(dbfp,"\t\tcomp = \"%s\";\n",device[i].comp[j].comp);
                    fprintf(dbfp,"\t\tnum = %i;\n",device[i].comp[j].num);
                    fprintf(dbfp,"\t\tname = \"%s\";\n",device[i].comp[j].name);
                    fprintf(dbfp,"\t\ttype = %i;\n",device[i].comp[j].type);
                    fprintf(dbfp,"\t\tvalue = %i;\n",device[i].comp[j].value);
                    fprintf(dbfp,"\t      };\n"); // close component
                    
                }//if
            }//for
            fprintf(dbfp,"\t};\n"); // close device
        }
    }//for i
    fclose(dbfp);
    
}//dump_db()


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
    
    int len = strlen(str);
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



