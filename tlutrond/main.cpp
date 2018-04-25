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
 
 main.cpp
 
 lutrond V4.0 April 2018
 
 Global vars and main()
 
 ***********/


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
                    DEFAULT_PORT,               //port
                    0,                          //sockfd
                    0,                          //actsockfd
                    0                           //connected
};

lutron_t lutron ={
                    (char *)LUTRON_HOST,     // host        lutron hostname
                    (char *)LUTRON_PORT,     // lport       telnet port number
                    (char *)LUTRON_USER,     // user        Lutron UID
                    (char *)LUTRON_PASS,     // password    Lutron password
                    0                        // fd          lutron FD for PTY
    
};

// the message queue is used to pass command messages between the thread that listens
// for client connections to the thread with telnet session to the Lutron
MessageQueue_t queue;                   // create in instance of a queue
MessageQueue_t *mq = &queue;            // and a pointer to this queue

lut_dev_t device[NO_OF_DEVICES];        // database of Lutron devices


//TODO. It seems best practice to dedicate a thread to singal traps and mask them out
//      of all other threads. Not doing this at present. It's undefined which thread
//      run these handlers, but as all they do is set riase further signals and/or
//      modify global flags, it's not overly important.
struct sigaction saCHLD,saHUP;          // two trap handlers. TODO combine
pid_t telnet_pid;

pthread_t   lutron_tid,     // controlling lutron thread - perpetually creates lutron_tid2's
            client_tid,     // socket listening thread. Accepts connections, takes in commands
                            // and pushes them to the message queue
            lutron_tid2;    // Lutron session. Forks a telnet session using pty and transacts
                            // commands off the queue.

/**********************************************************************
          MAIN
 *********************************************************************/
int main(int argc, const char *argv[]) {
    
  int thread_error;
  int i,opt;

    
#ifdef _ROOT_PRIV
    testRoot();         // Exit if not run by root
#endif
    // printLerrors();  // TODO reminder to parse lutron ~ERROR messages
    
    //Parse command line options
    while ((opt = getopt(argc, (char **)argv, "Dkhvdtp:c:")) != -1){
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
                
            case 'k': // kill, cycle Lutron login session (and dump database)
                flag.kill = true;
                if(flag.debug) printf("kill mode\n");
                break;
                
            case 'v': // version
                printf("%s\n",VERSION);
                exit(EXIT_SUCCESS);
                break;
            
            case 'h': // help
            default:
                usageError(argv[0]);
                break;
                
        }//switch
    }//while opt parse
    
    if(flag.debug)fprintf(stderr,"command ops parsed\n");
    if(flag.debug)fprintf(stderr,"logfile: %s\n",admin.log_file);
    
    
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
    
    pidFile(admin.pid_file,argstr(argc,(char **)argv)); // write pid file

    if(flag.debug){
        printf("conf_file = %s\n",admin.conf_file);
        printf("userid = %s\n",lutron.user);
        printf("log_file = %s\n",admin.log_file);
        printf("conf_file = %s\n",admin.conf_file);
    }
   
    // TODO impliment a signal handling thread for these. And mask these out of all other
    // threads
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
    
    // socket listener thread
    thread_error = pthread_create(&client_tid, NULL, &client_listen, NULL);
    if (thread_error != 0){
        logMessage("Socket thread creation failure");
        fprintf(stderr,"Can't create Client listen thread :[%s]\n", strerror(thread_error));
        exit(EXIT_FAILURE);
    }else{
        if(flag.debug) printf("main1:Thread created successfully\n");
    }
    
    // Lutron connection thread
    thread_error = pthread_create(&lutron_tid, NULL, &lutron_connection, NULL);
    if (thread_error != 0){
        logMessage("Lutron thread creation failure");
        fprintf(stderr,"Can't create Lutron thread :[%s]\n", strerror(thread_error));
        exit(EXIT_FAILURE);
    }else{
        if(flag.debug) printf("Main1:Thread created successfully\n");
    }
    
    // Main thread now loops and monitors
    usleep(10000000); // give the worker threads some time to set up socket & connection
    
    i=0;            // watch dog loop counter
    while(true){
        usleep(10000000);
        if(flag.debug)printf("[%i]main:ping\n",i);
        ++i;
        if( i % 10 == 0 ) keepAlive(); // tickle the queue every 10 loops
        if( i > 500 ){              // kill telnet every 500 loops
            flag.dump=true;         // and cause database to be dumped in so doing
            if (getpgid(telnet_pid) >= 0){ // crafty way to see if process exists
                kill(telnet_pid,SIGHUP);    // the forked session. Should terminate and
                                            // raise a SIGCHLD
                if(flag.debug) printf("main1:SIGHUP sent to telnet\n");
            }else{                          // re-thread telnet
                if(flag.debug) printf("main1:telnet not running\n");
                pthread_kill(lutron_tid2,SIGHUP);  // kill the thread that forked telnet
                            // when lutron_tid2 dies, lutron_tid will start another one
                
                if(flag.debug) printf("main2:Thread killed successfully\n");
                    // TODO .. we don't know this for sure as we havn't tested the
                    // return value of pthread_kill
            }
            i=0;
            flag.connected=false; // will cause telnet_tid2 to initiate lutron login process
                                  // probably done elswhere in the logic, but it doesn't hurt.
        }//if (i > 50)
    }//while true
   // NEVER REACHED
}


