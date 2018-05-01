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
 
 signals.cpp
 
 lutrond V4.0 April 2018
 
 sigchldHandler(), sighupHandler(), lutkill(), killTelnet(), signals_thread()
 
 ***********/


#include "lutrond.h"
#include "externals.h"


#define CMD_LINE_LEN 256

// SIGHUP and SIGCHLD are both blocked in subthreads leaving the main thread
// to handle both these.
//
// The handlers operate thus:
// SIGCHLD (i.e. telnet has died). set global flag.connected = false. This will cause
//      connection loop in lutron thread to break, the thread die and a new one envoked
// SIGHUP  Calls killTelnet which will in turn cause a SIGCHLD to be raised. See above.


//************   sigchldHandler
void
sigchldHandler(int sig)
{
    
    int status;
    
    if(flag.debug) printf("SIGCHLD trapped (handler)\n"); // UNSAFE
    logMessage("SIGCHLD received");
    if (flag.sigchld_ignore){
        logMessage("SIGCHLD ignored");
        flag.sigchld_ignore = false;
    }else{
        while( waitpid(-1,&status, WNOHANG) > 0){}; // waits for child to die
        flag.connected = false;     // force lutron_connect thread to break out of
                                    // connected loop and thence re-establish connection
    }//else
    
    
}//sigchldHandler

//************   sighupHandler
void
sighupHandler(int sig)
{
    
    // NOTE: xcode catches SIGHUP for debuging.
    
    // all we do here is kill telnet and then let the SIGCHLD handler do the
    // rest when it catches this.
    
    if(flag.debug) printf("SIGHUP trapped (handler)\n"); // UNSAFE
    logMessage("SIGHUP received");
    killTelnet();
    flag.dump=true; // cause db to be dumped next time it's tested.
    
    
}//sighupHandler

//************   sigtermHandler
void
sigtermHandler(int sig)
{
    sigset_t set;
    
    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    
    pthread_sigmask(SIG_BLOCK, &set, NULL); // BLOCK SIGCHLD in thread
    sigprocmask(SIG_BLOCK,&set,NULL);  // BLOCK SIGCHLD across process
    if(flag.debug) printf("SIGTERM trapped (handler)\n"); // UNSAFE
    logMessage("SIGTERM received");
    killTelnet();
    exit(EXIT_SUCCESS);
    
}//END***************   sigtermHandler

//************   lutkill()
int lutkill(const char *pid_filename) {  // -k routine
    
    static FILE *pidfp;
    int pid;
    char command_line[CMD_LINE_LEN];
    char **exec_args;        // command line args for respawn
    
    pidfp = fopen(pid_filename, "r");
    
    if (pidfp == NULL){
        return (EXIT_FAILURE);
    };
    
    // the pid file has
    // Line 1: running lutrond process ID
    // Line 2: the command line used to invoke it
    
    if(fscanf(pidfp,"%d\n%[^\n]s",&pid,command_line) == -1){
        return (EXIT_FAILURE);
    }
    if (getpgid(pid) >= 0){ // crafty way to see if process exists
        
        if (kill(pid, SIGHUP) == -1) {
            return (EXIT_FAILURE);
        }
        
        return (EXIT_SUCCESS);
    }// no such process so we should exec another
    
    // exec a new lutrond.
    
    // convert string version of command line to array of pointers to strings
    exec_args = strarg(command_line);
    // strip any path head off the first arg (as in argv[0])
    exec_args[0] = basename(exec_args[0]);
    // We assume the process will daemonize per the -d
    // flag that will enevitably be found in command_line
    // TODO a more complete solution would be to fork/exec/die
    // to disconnect this instance as we respawn
    execvp("/usr/local/bin/lutrond",(char **)exec_args);
    
    // exec failed!
    
    logMessage("lutrond -k failed to exec process");
    
    return (EXIT_FAILURE); // if we get here its proper broke
    
}
//END************   lutkill


//************   killTelnet()
void killTelnet(){
    
    int status;
    
    if (getpgid(telnet_pid) >= 0){  // crafty way to see if process exists
        kill(telnet_pid,SIGKILL);    // the forked session. Will terminate and
        // raise a SIGCHLD, which will cause
        // lutron_tid2 to end and be recreated by
        // lutron_tid
        if(flag.debug) fprintf(stderr,"killTelnet: SIGTERM sent to telnet\n");
        waitpid(telnet_pid, &status, WNOHANG);
    }else{                          // re-thread telnet
        if(flag.debug) fprintf(stderr,"killTelnet: telnet not running\n");
        pthread_kill(lutron_tid2,SIGTERM);  // kill lutron_tid2, the thread that
        // forked telnet. When lutron_tid2 dies, lutron_tid will start another one
        
        if(flag.debug) fprintf(stderr,"killTelnet: Lutron thread killed successfully\n");
        // TODO .. we don't know this for sure as we havn't tested the
        // return value of pthread_kill
    }
    
}
//END************   killTelnet

//************   signals_thread()
void* signals_thread(void *arg){
    
    struct sigaction saCHLD,saHUP,saTERM;
    
    sigset_t set;
    
    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    sigaddset(&set, SIGHUP);
    sigaddset(&set, SIGTERM);
    pthread_sigmask(SIG_UNBLOCK, &set, NULL);
    
    //  Install SIGCHLD handler
    if(flag.debug)fprintf(stderr,"Loading SIGCHLD handler\n");
    sigemptyset(&saCHLD.sa_mask);
    saCHLD.sa_flags = SA_RESTART;
    saCHLD.sa_handler = sigchldHandler;
    if (sigaction(SIGCHLD, &saCHLD, NULL) == -1){
        error("Error loading SIGCHLD signal handler");
    }//if
    
    //  Install SIGHUP handler
    if(flag.debug)fprintf(stderr,"Loading SIGHUP handler\n");
    sigemptyset(&saHUP.sa_mask);
    saHUP.sa_flags = SA_RESTART ;
    saHUP.sa_handler = sighupHandler;
    if (sigaction(SIGHUP, &saHUP, NULL) == -1){
        error("Error loading HUP signal handler");
    }//if
    
    //  Install SIGTERM handler
    if(flag.debug)fprintf(stderr,"Loading SIGTERM handler\n");
    sigemptyset(&saTERM.sa_mask);
    saTERM.sa_flags = SA_RESTART ;
    saTERM.sa_handler = sigtermHandler;
    if (sigaction(SIGTERM, &saTERM, NULL) == -1){
        error("Error loading TERM signal handler");
    }//if
    
//***************** MAIN LOOP
    while(true){ // just suspend awaiting signals
        
        sigsuspend(&set);
        
    }
}
//END*********** signals_thread
