/*
main.cpp    ->  tlutrond

Created by Ed Cole on 19/04/2018.
Copyright © 2018 Ed Cole. All rights reserved.

threaded version of lutrond

*/

#include "lutrond.h"

// Type defs and structs defined in lutrond.h

// Global variable declarations (include these in externals.h)

flags_t flag = {
                    false,          // daemon
                    _DEBUG,         //debug
                    _TEST_MODE,     // test
                    false,          // connected
                    false,          // dump
                    false,          // kill
                    false           // port
};

daemon_t admin = {  0,                          // pid
                    (char *)CONFIG_FILE,        // conf_file
                    (char *)LOG_FILE_NAME,      // log_file
                    (char *)PID_FILE_NAME,      // pid_file
                    (char *)DB_FILE_NAME,       // db_file
                    NULL                        // logfp
};

client_t listener = {
                    DEFAULT_PORT       //port
};

lutron_t lutron ={
                    (char *)LUTRON_HOST,     // lutron hostname
                    (char *)LUTRON_PORT,     // telnet port number
                    (char *)LUTRON_USER,     // Lutron UID
                    (char *)LUTRON_PASS,     // Lutron password
                    0                        // lutron FD for PTY (was lutron)
    
};


MessageQueue_t queue;                   // create in instance of a queue
MessageQueue_t *mq = &queue;            // and a pointer to this queue

lut_dev_t device[NO_OF_DEVICES];        // database of Lutron devices

jmp_buf JumpBuffer;                     // for non-local goto in signal traps
struct sigaction saCHLD,saHUP;          // two trap handlers. TODO combine
pid_t telnet_pid;

  pthread_t lutron_tid,client_tid,lutron_tid2;                      // Thread IDs

/**********************************************************************
          MAIN
 *********************************************************************/
int main(int argc, const char *argv[]) {
    

  int thread_error;
    // int status;
    
    int i,opt;

    testRoot();    // Exits if not run by root
    printLerrors();
    
    //Parse command line options
    while ((opt = getopt(argc, (char **)argv, "Dkvdtp:c:")) != -1){
        switch (opt){
                
            case 'D': // run in debug mode. Use as first option to catch all in this
                // switch block
                flag.debug=true;
                printf("Debug mode set\n");
                break;
                
            case 'd': // Run as a daemon
                if(flag.debug) printf("Running as a Daemon\n");
                becomeDaemon((int)(   BD_NO_CLOSE_FILES |
                                        BD_NO_CHDIR |
                                        BD_NO_REOPEN_STD_FDS));
                break;
                
            case 'c': // Alternate config file
                admin.conf_file=(char *)malloc(sizeof(optarg));
                strcpy(admin.conf_file,optarg);
                break;
                
            case 'p': // Alternate listening port
                listener.port = atoi(optarg);
                flag.port=true; // overide default & config file directive
                break;
                
            case 't': // test mode, no Lutron connection
                flag.test = true;
                if(flag.debug) printf("test mode\n");
                break;
                
            case 'k': // test mode, no Lutron connection
                flag.kill = true;
                if(flag.debug) printf("kill mode\n");
                break;
                
            case 'v': // version
                printf("%s\n",VERSION);
                exit(EXIT_SUCCESS);
                break;
                
            default:
                //usageError(argv[0]);
                break;
                
        }//switch
    }//while opt parse
    
    if(flag.debug)printf("command ops parsed\n");
    if(flag.debug)printf("logfile: %s\n",admin.log_file);
    
    
    if (readConfFile(admin.conf_file)==EXIT_FAILURE){
        fprintf(stderr,"Running without conf file\n");
    };
    
    logOpen(admin.log_file);
    logMessage("Starting:%s",(char *)argstr(argc,(char **)argv));
    
    if( flag.kill ){ // send HUP to existing process or respawn
        if(lutkill(admin.pid_file)==EXIT_SUCCESS){
            if(flag.debug) printf("kill succeded\n");
            logMessage("[lutrond -k]  succeded\n");
            exit(EXIT_SUCCESS);
        }//if
        // Kill failed
        logMessage("[lutrond -k] kill -HUP attempt failed\n");
        exit(EXIT_FAILURE);
    }// if kill_flag
    
    openlog(SYSLOG_IDENT,SYSLOG_OPT,SYSLOG_FACILITY); // SYSLOG open
    syslog(SYSLOG_OPT,"startup. Also see %s",admin.log_file);
    
    pidFile(admin.pid_file,argstr(argc,(char **)argv));

    if(flag.debug){
        printf("conf_file = %s\n",admin.conf_file);
        printf("userid = %s\n",lutron.user);
        printf("log_file = %s\n",admin.log_file);
        printf("conf_file = %s\n",admin.conf_file);
    }
   
   
    //  Install SIGCHLD handler
    sigemptyset(&saCHLD.sa_mask);
    saCHLD.sa_flags = SA_RESTART ;
    saCHLD.sa_handler = sigchldHandler;
    if (sigaction(SIGCHLD, &saCHLD, NULL) == -1){
        error("Error loading SIGCHLD signal handler");
    }//if
    
    
    //  Install SIGHUP handler
    sigemptyset(&saHUP.sa_mask);
    saHUP.sa_flags = SA_RESTART ;
    saHUP.sa_handler = sighupHandler;
    if (sigaction(SIGHUP, &saHUP, NULL) == -1){
        error("Error loading HUP signal handler");
    }//if
   

    // create the worker threads
    thread_error = pthread_create(&client_tid, NULL, &client_listen, NULL);
    if (thread_error != 0)
        fprintf(stderr,"Can't create Client listen thread :[%s]\n", strerror(thread_error));
    else
        if(flag.debug) printf("main1:Thread created successfully\n");
    
    
    
    
    
    thread_error = pthread_create(&lutron_tid, NULL, &lutron_connection, NULL);
    if (thread_error != 0){
        fprintf(stderr,"Can't create Lutron thread :[%s]\n", strerror(thread_error));
        exit(EXIT_FAILURE);
    }
    if(flag.debug) printf("Main1:Thread created successfully\n");
    
    i=0;
    while(true){
        usleep(10000000);
        if(flag.debug)printf("[%i]main:ping\n",i);
        ++i;
        if( i > 500 ){ // kill telnet every 500 loops
            flag.dump=true;
            if (getpgid(telnet_pid) >= 0){ // crafty way to see if process exists
                kill(telnet_pid,SIGHUP);
                if(flag.debug) printf("main1:SIGHUP sent to telnet\n");
            }else{ // re-thread telnet
                if(flag.debug) printf("main1:telnet not running\n");
                pthread_kill(lutron_tid2,SIGHUP);
                /* Shouldn't rethread lutron_connection - as existing thread is joined to
                        lutron_tid2, so will rethread when lutron_tid2 dies.
                thread_error = pthread_create(&lutron_tid, NULL, &lutron_connection, NULL);
                if (thread_error != 0){
                    fprintf(stderr,"main2:Can't create Lutron thread :[%s]\n", strerror(thread_error));
                    exit(EXIT_FAILURE);
                }*/
                
                if(flag.debug) printf("main2:Thread killed successfully\n");
            }
            i=0;
            flag.connected=false;
        }//if (i > 50)
    }//while true
   // NEVER REACHED
}

