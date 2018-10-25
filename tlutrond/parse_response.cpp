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
 
 parse_response.cpp
 
 lutrond V4.0 April 2018
 
 parse_response() runScript()
 
 ***********/

#include "lutrond.h"
#include "externals.h"

// array of strings describing Lutron device types by number
#define _NO_OF_TYPES 18
std::string types[_NO_OF_TYPES] = {
    "",
    "switched bulb",        //1
    "dimmed bulb",          //2
    "blind/curtain motor",  //3
    "fan",                  //4
    "3 key pad",            //5
    "5 key pad",            //6
    "10 key pad",           //7
    "virtual key pad",      //8
    "digital input module", //9
    "8 key pad",            //10
    "Wireless remote",      //11
    "Intruder",             //12
    "PIR inputs",           //13
    "Areas",                //14
    "Timeclocks",           //15
    "variables",            //16
    "sequences"             //17
};

int runScript(char *cmd){
    
    // I have lifted this replacement for the system() function, with some minor edits
    // from the book "Advanced Programming in the UNIX Environment" (fig 10.28) as it
    // seems to handle SIGCHLD better than the library call which was causing issues
    // with our SIGCHLD handler semantics. These, I believe are rooted in doing this in
    // threaded code, but havn't investigated thuroughly in the spirit of not trying to
    // fix something that isn't broken, or at least doesn't seem to be too broken ;-)
    
    // Now using simple flag to tell our SIGCHLD handler to ignore the signal. See below.
    
    pid_t                   pid;
    int                     status;
    struct sigaction        ignore, saveintr, savequit;
    sigset_t                chldmask, savemask;
    
    if (cmd == NULL)
        return(EXIT_FAILURE);
    flag.sigchld_ignore = true; // a fudge. This flags my SIGCHLD handler, which is more concerned
                                // with the death of telnet than this child process to ingore SIGCHLD
                                // while this script runs. Not ideal mechanism. i.e. consider if the
                                // script was to kill telnet ;-)
    
                                // The flag is reset to false by the handler, so we only ignore one
                                // signal instance
    
    logMessage("Runscript: %s", cmd);
    
    ignore.sa_handler = SIG_IGN;    /* ignore SIGINT and SIGQUIT */
    sigemptyset(&ignore.sa_mask);
    ignore.sa_flags = 0;
    if (sigaction(SIGINT, &ignore, &saveintr) < 0)
        return(EXIT_FAILURE);
    if (sigaction(SIGQUIT, &ignore, &savequit) < 0)
        return(EXIT_FAILURE);
    sigemptyset(&chldmask);            /* now block SIGCHLD */
    sigaddset(&chldmask, SIGCHLD);
    if (pthread_sigmask(SIG_BLOCK, &chldmask, &savemask) < 0)
        return(EXIT_FAILURE);
    
    if ((pid = fork()) < 0) {
        status = EXIT_FAILURE;    /* probably out of processes */
    } else if (pid == 0) {            /* child */
        /* restore previous signal actions & reset signal mask */
        sigaction(SIGINT, &saveintr, NULL);
        sigaction(SIGQUIT, &savequit, NULL);
        pthread_sigmask(SIG_SETMASK, &savemask, NULL);
        
        execl("/bin/sh", "sh", "-c", cmd, (char *)0);
        _exit(127);        /* exec error */
    } else {                        /* parent */
        while (waitpid(pid, &status, 0) < 0)
            if (errno != EINTR) {
                status = EXIT_FAILURE; /* error other than EINTR from waitpid() */
                break;
            }
    }
    
    /* restore previous signal actions & reset signal mask */
    if (sigaction(SIGINT, &saveintr, NULL) < 0)
        return(EXIT_FAILURE);
    if (sigaction(SIGQUIT, &savequit, NULL) < 0)
        return(EXIT_FAILURE);
    if (pthread_sigmask(SIG_SETMASK, &savemask, NULL) < 0)
        return(EXIT_FAILURE);
    
    return(status); // which is the return code from the script
}


/*********** Version using system()
void runScript(char *cmd){
 
    logMessage("Runscript: %s", cmd);
    system(cmd);   // run the script
    
}
***********/

//************   parse_response
void parse_response(char *pre, char *pb){ // pre - prefix, pb buffer to parse
    
    // Looking for string with following format
    //  [#~]{OUTPUT,DEVICE},<device>,<command>,<param 1>,....<paramN>
    //  Other commands to manage include ERROR SYSVAR and TIMECLOCK
    
    char *tok,*cmd,*arg1,*arg2,*tmp;
    static char line[BUFFERSZ];             //TODO worry about static buffer and threads
    static char undef[] = "UNDEF";
    char *ptr1=NULL; // temp pointers for strtok calls
    char *ptr2=NULL;
    int dev = 0;
    int inform = 0;
    int component,action;
    char *param;
    int n,a;
    int found;
    static int tx_count = 0;  // transaction count TODO ditto above w/ threads
    char *script; // pointer to string containing script to fork
    
    
    bzero(line,BUFFERSZ);
    memcpy(line,pb,strlen(pb));
    
    tok = strtok_r(line,"\r\n\0",&ptr1);    // all the ways a string might
                                            // be terminated
                                            // and in case we have multiple
                                            // lines in buffer
    while( tok != NULL ){                   // tok holds whole line
        switch(tok[0]){
            case 'Q':
                // logMessage("QNET:>>");
                
                break;
                
            case '~': inform = 1;
                tx_count+=2;  // Increment counter (by 2 due to -1 drop thru)
            case '#':
            case '?': --tx_count;   // Decrement counter
                cmd = (tmp=strtok_r(tok,",",&ptr2)) == NULL ? undef : tmp ; // first token
                dev = (tmp=strtok_r(NULL,",",&ptr2)) == NULL ? 0 : atoi(tmp); // device
                arg1 = (tmp=strtok_r(NULL,",",&ptr2)) == NULL ? undef : tmp;
                // I think we are happy with this even though some command dont have a 4th token
                arg2 = (tmp=strtok_r(NULL,",",&ptr2)) == NULL ? undef : tmp;
                if(device[dev].name == NULL){ // we don't know about this device
                    device[dev].name = undef;
                    device[dev].location = undef;
                    device[dev].type=99; // or should I set to 99 here and be done with it?
                }
                switch (tok[1]){
                    case 'O':  // OUTPUT COMMAND
                        logMessage("[%i]%s:%s:%s(%s,%i,%s,%s)",
                                    tx_count,
                                    pre,
                                    device[dev].name,
                                    device[dev].location,
                                    cmd,
                                    dev,
                                    arg1,
                                    arg2);
                    break;
                        
                    case 'D':  // Device COMMAND arg1 is the component field
                        n=1; a=atoi(arg1);found=false;
                        while(device[dev].comp[n].num != 0){
                            if(device[dev].comp[n].num == a){
                                found=true;
                                // comp[n] is the component
                                // logMessage("DATABASE HIT (DEVICE)");
                                break;
                            }//if
                            ++n;
                        }//while
                        if(found){
                            logMessage("[%i]%s:%s:%s[%s](%s,%i,%s,%s)",
                                       tx_count,
                                       pre,
                                       device[dev].name,
                                       device[dev].location,
                                       device[dev].comp[n].name,
                                       cmd,
                                       dev,
                                       arg1,
                                       arg2);
                        }else{
                              logMessage("[%i]%s:%s:%s(%s,%i,%s,%s)",
                                   tx_count,
                                   pre,
                                   device[dev].name,
                                   device[dev].location,
                                   cmd,
                                   dev,
                                   arg1,
                                   arg2);
                        }// else
                    break;
                        
                    default:
                                logMessage("[%i]%s:%s:%s(%s,%i,%s,%s)",
                                           tx_count,
                                           pre,
                                           device[dev].name,
                                           device[dev].location,
                                           cmd,
                                           dev,
                                           arg1,
                                           arg2);
                     break;
                }//switch tok[1]
                    
                if (inform){
                    if (tok[1] == 'O'){ //~OUTPUT
                        action = atoi(arg1);
                        //component = 0; // there are no components
                        param = arg2;
                        switch ( device[dev].type ){
                            case (0): // unknown
                                device[dev].type = 99;
                                break;
                            case ( 1 ): // switched
                            case ( 2 ): // dimmer
                                if(action==1){//switching
                                    strcpy(device[dev].state,param);
                                    //logMessage("DATABASE HIT(1)");
                                }
                                if((action==29)||(action==30)){//LED perhaps
                                    //logMessage("DATABASE HIT(29/30)");
                                }
                        
                                break;
                                
                            default:
                                strcpy(device[dev].state,param);
                                // logMessage("UNKNOWN DEVICE HIT(1)");
                                
                                
                                break;
                                
                                
                        }//switch
                    }// if ~OUTPUT
                    if (tok[1] == 'D'){ //~DEVICE
                        component = atoi(arg1);
                        action = atoi(arg2);
                        // ~DEVICE,<dev>,<component>,<action>
                        found=FALSE;
                        n=1;
                        while(device[dev].comp[n].num != 0){
                            if(device[dev].comp[n].num == component){
                                device[dev].comp[n].value=action;
                                found=TRUE;
                                // logMessage("DATABASE HIT (DEVICE)");
                                break;
                            }//if
                            ++n;
                        }//while
                        if(n > NO_OF_COMPS){ // too many found
                            logMessage("Too many components");
                        }else{ // first we've seen of this component
                            if(!found){
                                device[dev].comp[n].num = component;
                                device[dev].comp[n].value = action;
                                // logMessage("DATABASE HIT (DEVICE) NEW");
                            }// if !found
                        }//if
                        if(device[dev].comp[n].type==9){ // SCRIPT
                            // device[dev].comp[n].comp is the script
                            // arg2 is the parameter to pass
                            script = (char *)malloc((int)strlen(device[dev].comp[n].comp)
                                            + (int)strlen(arg2)
                                            +2);     // +2 cos space separator an \0
                            sprintf(script,"%s %s",device[dev].comp[n].comp,arg2);
                            runScript(script);
                            free(script);
                        }
                    }// if ~DEVICE
                    
                    if (tok[1] == 'S'){ //~SYSVAR
                        component = atoi(arg1); // tycically = 1
                        action = atoi(arg2);    // AKA value
                        
                        // ~SYSVAR,<dev>,<action>,<value>  action = 1 always
                        found=FALSE;
                        n=1;
                    }// if ~SYSVAR
                    
                }// if inform
                inform = 0;
                
                break;
                
            default:
                
                break;
                
        }//switch
        tok = strtok_r(NULL,"\r\n\0",&ptr1);
        
    }//while
    return;
}// parse_response

