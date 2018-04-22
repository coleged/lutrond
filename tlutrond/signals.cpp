//
//  signals.cpp
//  tlutrond
//
//  Created by Ed Cole on 22/04/2018.
//  Copyright © 2018 Ed Cole. All rights reserved.
//

#include "lutrond.h"
#include "externals.h"


#define CMD_LINE_LEN 256

#define HUP 1



//************   sigchldHandler
void
sigchldHandler(int sig)
{
    
    int status;
    
    if(flag.debug) printf("SIGCHLD trapped (handler)\n"); // UNSAFE
    logMessage("SIGCHLD received");
    while( waitpid(-1,&status, WNOHANG) > 0){}; // waits for child to die
    close(lutron.fd);
    
    // TODO
    //siglongjmp(JumpBuffer,1); //  go to re-fork another Lutron connection
    
}//sigchldHandler

//************   sighupHandler
void
sighupHandler(int sig)
{
    
    // all we do here is kill child and then let the SIGCHLD handler do the
    // rest when it catches this.
    
    if(flag.debug) printf("SIGHUP trapped (handler)\n"); // UNSAFE
    logMessage("SIGHUP received");
    flag.dump=TRUE; // cause db to be dumped when SIGCHLD longjumps
    // kill(pid,SIGKILL); // TODO Kill lutron thread
    // never gets here as the SIGKILL is trapped and long jumped back into
    // main()
    
    
}//sighupHandler



int lutkill(const char *pid_filename) {
    
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

