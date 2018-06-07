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
 
  lutrond.h
 
lutrond V4.0 April 2018

***********/

#include "build.h"


#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <mutex>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/types.h>
#include <ctype.h>
#include <sys/socket.h>

#ifdef MAC_OS10		    // Mac OS doesn't have pty.h and header search path is broken
#include <util.h>
#include "/usr/local/include/libconfig.h"
#else
#include <pty.h>
#include <libconfig.h>
#endif

#ifdef LINUX
#include <sys/prctl.h>      // prctl is used to set parent death signal (PR_SET_PDEATHSIG)
#endif                      // of telnet process to SIGTERM to stop zombie creation

#include <termios.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <netinet/in.h>

#include <time.h>
#include <syslog.h>
#include <libgen.h>    // for basename()
#include "become_daemon.h"
#include "tlpi_hdr.h"		// uses some code snarfed from TLPI book
#undef		min
#undef		max
#include "unp.h"		// uses some code snarfed from the UNP book


#define MY_NAME "lutrond"
#define VERSION "4.0.0"

#define TRUE	1
#define FALSE	0

#define BUFFERSZ 4096    	// buffer for IPC - big because we dont
				            // do any significant flow control
#define PIPE_BUFFER 4096

#define NO_OF_DEVICES 256	// max # of Lutron devices
#define NO_OF_COMPS 32		// max # of Components per device


// defaults. Some of these can be overridden in the lutrond.conf file   (*)
// or via command line option flags                                     (+)
#define LUTRON_HOST "lutron1.rainbow-futures.com"       // *+
#define LUTRON_PORT "23"				                // *
#define LUTRON_USER "lutron"				            // *
#define LUTRON_PASS "integration"			            // *
#define SYSLOG_IDENT    "lutrond"
#define LOG_FILE_NAME   "/tmp/lutrond.log"		        // *
#define DB_FILE_NAME	"/tmp/lutrond.db"
#define PID_FILE_NAME   "/tmp/lutrond.pid"

#ifdef MAC_OS10        // Mac OS high sierra (re)moved telnet
#define TELNET_PROG     "/usr/local/bin/telnet"
#else
#define TELNET_PROG     "/usr/bin/telnet"
#endif

#define SYSLOG_OPT (LOG_PID | LOG_NDELAY | LOG_NOWAIT)
#define SYSLOG_FACILITY LOG_LOCAL7
#define CONFIG_FILE     "/usr/local/etc/lutrond.conf"	// +
#define DEFAULT_PORT    4534                            // *+
#define SOC_BL_QUEUE    5
#define TS_BUF_SIZE     sizeof("YYYY-MM-DD HH:MM:SS")


typedef struct{
    bool daemon;
    bool debug;
    bool test;
    bool connected;
    bool dump;
    bool kill;
    bool port;
    bool sigchld_ignore;
}flags_t;

typedef struct{
            int port;
            int sockfd;
            int actsockfd;
            bool connected;
    
}client_t;   // client socket

typedef struct{
    char *host;     // lutron hostname
    char *lport;      // telnet port number
    char *user;
    char *password;
    int fd;     // lutron FD for PTY (was lutron)
}lutron_t;

typedef struct{
    pid_t pid;
    char *conf_file;
    char *log_file;
    char *pid_file;
    char *db_file;
    FILE *logfp;
}daemon_t;            // daemon config parameters

// TODO - rewrite all this using a database!!!!!!!
// Array of Lutron Devices index is what Lutron call Integration ID
typedef struct {
        char *name;     // Friendly name for device
        char *location;
        int type;       // Switched lamp, blind, dimmer circuit etc.
        char state[7];      //  0% 100% or other - use #define Constants
          struct{
            int empty;	// 1st  comp. 1 =  there are no components modeled on this device 
            char *comp;  // Lutron component type or script
            int num;     // Lutron as they are all over the shop
            int type;    // LED, Button, Bulb, Dimmer, script(9) etc
            char *name;  // Friendly name for component
            int value;   // state, i.e. 0 for off/disable, 100 for 100% on etc
           } comp[NO_OF_COMPS]; //  1 to NO_OF_COMPS-1 (ignore 0)
       } lut_dev_t;

// function prototypes
void logMessage(const char *format, ...);
void logOpen(const char *logFilename);

char *lerror(int number);       // lerror.c returns pointer to string containing
                                // description of Lutron error
void beat_test();
int lutkill(const char *);
char *argstr(int, char **); // args to string
char **strarg(char *);		   // string to args
int testRoot();
void* client_listen(void *);
void* lutron_connection(void *);
void error(const char *);
int readConfFile(char *);
void pidFile(const char *,char *);
void parse_response(char *, char *);
char *db_timestamp();
void sigchldHandler(int);
void sighupHandler(int);
void sigtermHandler(int);
int dump_db();
void keepAlive();
void usageError(const char *);
void killTelnet();
void* signals_thread(void *);
/**********************  END END END ***********************************/
