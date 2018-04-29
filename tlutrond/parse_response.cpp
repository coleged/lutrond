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

void runScript(char *path){
    
    logMessage("Runscript: %s", path);
    system(path);
    
    // TODO. SIGCHLD is blocked while system() call runs...
    // but this looks like it's just for this thread as the main thread
    // catches a SIGCHLD and kills the lutron connection as it should!!!
    
}


//************   parse_response
void parse_response(char *pre, char *pb){ // pre - prefix, pb buffer to parse
    
    // Looking for string with following format
    //  [#~]{OUTPUT,DEVICE},<device>,<command>,<param 1>,....<paramN>
    //  Other commands to manage include ERROR and TIMECLOCK
    
    char *tok,*cmd,*arg1,*arg2,*tmp;
    static char line[BUFFERSZ];             //TODO worry about static buffer and threads
    static char undef[] = "UNDEF";
    char *ptr1=NULL; // temp pointers for strtok calls
    char *ptr2=NULL;
    int dev = 0;
    int inform = 0;
    int component,action;
    char *param;
    int n;
    int found;
    static int tx_count = 0;  // transaction count TODO ditto above w/ threads
    
    bzero(line,BUFFERSZ);
    memcpy(line,pb,strlen(pb));
    
    tok = strtok_r(line,"\r\n\0",&ptr1);     // all the ways a string might
    // be terminated
    // and in case we have multiple
    // lines in buffer
    while( tok != NULL ){
        switch(tok[0]){
            case 'Q':
                // logMessage("QNET:>>");
                
                break;
                
            case '~': inform = 1;
                tx_count+=2;  // Increment counter (by 2 due to -1 drop thru)
            case '#':
            case '?': --tx_count;   // Decrement counter
                cmd = (tmp=strtok_r(tok,",",&ptr2)) == NULL ? undef : tmp ; // first token
                dev = (tmp=strtok_r(NULL,",",&ptr2)) == NULL ? 999 : atoi(tmp); // device
                arg1 = (tmp=strtok_r(NULL,",",&ptr2)) == NULL ? undef : tmp;
                
                arg2 = (tmp=strtok_r(NULL,",",&ptr2)) == NULL ? undef : tmp;
                
                logMessage("[%i]%s:%s:%s(%s,%i,%s,%s)",
                           tx_count,
                           pre,
                           device[dev].name,
                           device[dev].location,
                           cmd,
                           dev,
                           arg1,
                           arg2);
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
                            runScript(device[dev].comp[n].comp); // the path to script
                        }
                    }// if ~DEVICE
                    
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

