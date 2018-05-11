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

// Global variable declarations (these should be exported by inclusion in externals.h)

flags_t flag = {
                    false,          // daemon
                    _DEBUG,         //debug
                    _TEST_MODE,     // test
                    false,          // connected
                    false,          // dump
                    false,          // kill
                    false,           // port
                    false           // sigchld_ignore
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


lut_dev_t device[NO_OF_DEVICES];        // database of Lutron devices

pid_t telnet_pid;

pthread_t   lutron_tid,     // controlling lutron thread - perpetually creates lutron_tid2's
            client_tid,     // socket listening thread. Accepts connections, takes in commands
                            // and pushes them to the message queue
            lutron_tid2,    // Lutron session. Forks a telnet session using pty and transacts
                            // commands off the queue.
            sig_tid;        // signal handling thread

int pfd[2];                // pipe for inter thread communications

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
                fprintf(stderr,"Debug mode set\n");
                break;
                
            case 'd': // Run as a daemon
                if(flag.debug) fprintf(stderr,"Running as a Daemon\n");
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
                if(flag.debug) fprintf(stderr,"test mode\n");
                break;
                
            case 'k': // kill, cycle Lutron login session (and dump database)
                flag.kill = true;
                if(flag.debug) fprintf(stderr,"kill mode\n");
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
            logMessage("[lutrond -k]  succeded");
            exit(EXIT_SUCCESS);
        }//if
        // Kill failed
        logMessage("[lutrond -k] kill -HUP attempt failed");
        error("kill failed");
    }// if kill_flag
    
    openlog(SYSLOG_IDENT,SYSLOG_OPT,SYSLOG_FACILITY); // SYSLOG open
    syslog(SYSLOG_OPT,"startup. Also see %s",admin.log_file);
    
    pidFile(admin.pid_file,argstr(argc,(char **)argv)); // write pid file

    if(flag.debug){
        fprintf(stderr,"conf_file = %s\n",admin.conf_file);
        fprintf(stderr,"userid = %s\n",lutron.user);
        fprintf(stderr,"log_file = %s\n",admin.log_file);
        fprintf(stderr,"conf_file = %s\n",admin.conf_file);
    }
    
    // create pipe for inter-thread messages
    
    if (pipe(pfd) < 0){
        logMessage("main():failed to create pipe");
        exit(EXIT_FAILURE);
    }
    if(flag.debug)fprintf(stderr,"created pipe pfd[0](read)=%i / pfd[1](write)=%i\n",pfd[0],pfd[1]);
   
    // signal handling thread
    thread_error = pthread_create(&sig_tid, NULL, &signals_thread, NULL);
    if (thread_error != 0){
        logMessage("Signals thread creation failure :[%s]",strerror(thread_error));
        error("Signals thread creation");
    }else{
        if(flag.debug) fprintf(stderr,"main1:Signals thread created successfully\n");
    }
    
    // worker threads
    
    // socket listener thread
    thread_error = pthread_create(&client_tid, NULL, &client_listen, NULL);
    if (thread_error != 0){
        logMessage("Client socket thread creation failure :[%s]",strerror(thread_error));
        error("Client socket thread creation");
    }else{
        if(flag.debug) fprintf(stderr,"main1:Client Listen thread created successfully\n");
    }
    
    // Lutron connection thread
    thread_error = pthread_create(&lutron_tid, NULL, &lutron_connection, NULL);
    if (thread_error != 0){
        logMessage("Lutron socket thread creation failure :[%s]",strerror(thread_error));
        error("Lutron socket thread creation");
    }else{
        if(flag.debug) fprintf(stderr,"Main1:Lutron thread created successfully\n");
    }
    
    // main thread
    
    dump_db();
    // Main thread just loops, sending keep alives and doing reset/connect
    usleep(1E4); // give the worker threads some time to set up socket & connection
    
    i=0;            // watchdog loop counter
    while(true){
        usleep(1E7); // zzzzzzzzzzz
        if(flag.debug)fprintf(stderr,".");
        ++i;
        if( i % 10 == 0 ) keepAlive();  // tickle lutron every 10 loops
        if( i > 500 ){                  // dump and reconnect every 500
            dump_db();                      // dump the database
            killTelnet();                   // and kill the telnet process
            i=0;
        }//if
    }//while true
   // NEVER REACHED
}


