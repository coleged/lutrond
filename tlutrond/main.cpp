/*
main.cpp    ->  tlutrond

Created by Ed Cole on 19/04/2018.
Copyright © 2018 Ed Cole. All rights reserved.

threaded version of lutrond

*/

#include "lutrond.h"


// Global declarations (include these in externals.h)
int tcount = 0;                     // thread counter - simple wait mechanism for now
// OK for thread finnishing normally, but won't
// work if thread dies abnornally
std::mutex tlock;                   // general purpose thread lock
// TODO better thread monitor. All this does at present is
// increment counter when thread starts, decrement when ends
// main thread loops until count == 0

flags_t flag = {
                    false,          // daemon
                    _DEBUG,         //debug
                    _TEST_MODE,     // test
                    false,          // connected
                    false,          // dump
                    false           // kill
};

daemon_t admin = {  0,                          // pid
                    (char *)LOG_FILE_NAME,      // log_file
                    (char *)PID_FILE_NAME,      // pid_file
                    (char *)DB_FILE_NAME,       // db_file
                    NULL                        // logfp
};

bool debug = flag.debug;              // TODO tidy out references to "debug"
MessageQueue_t queue;                 // create in instance of a queue
MessageQueue_t *mq = &queue;          // and a pointer to this queue
lut_dev_t device[NO_OF_DEVICES];    // database of Lutron devices

jmp_buf JumpBuffer;        // for non-local goto in signal traps
struct sigaction saCHLD,saHUP;      // two trap handlers. TODO combine

char d_host[]                = LUTRON_HOST;
const char *host     = d_host;
char d_tport[]                = LUTRON_PORT;
const char *tport     = d_tport;
char d_userid[]             = LUTRON_USER;
const char *userid     = d_userid;
char d_password[]            = LUTRON_PASS;
const char *password     = d_password;
char d_log_file[]             = LOG_FILE_NAME;
const char *log_file     = d_log_file;
char d_db_file[]                       = DB_FILE_NAME;
const char *db_file    = d_db_file;
char d_pid_file[]                       = PID_FILE_NAME;
const char *pid_file    = d_pid_file;

pid_t pid;             // child (telnet) process ID

int not_connected = 1;

int test_mode = _TEST_MODE;
int port=0;
static FILE *logfp;
int lutron;        // file handle for telnet to Lutron
int dump_db_flag = FALSE;
int kill_flag = FALSE;     // set true is this is a killer instance


/**********************************************************************
          MAIN
 *********************************************************************/
int main(int argc, const char * argv[]) {
    
  pthread_t lutron_tid,client_tid;                      // Thread IDs
  int thread_error;
    
    int i,opt;
    char *conf_file=NULL;
    
    
    
    testRoot();    // Exits if not run by root
    printLerrors();
    
    //Parse command line options
    while ((opt = getopt(argc, (char **)argv, "Dkvdtp:c:")) != -1){
        switch (opt){
                
            case 'D': // run in debug mode. Use as first option to catch all in this
                // switch block
                debug=TRUE;
                printf("Debug mode set\n");
                break;
                
            case 'd': // Run as a daemon
                if(debug) printf("Running as a Daemon\n");
                becomeDaemon(  (int)(   BD_NO_CLOSE_FILES |
                                        BD_NO_CHDIR |
                                        BD_NO_REOPEN_STD_FDS));
                break;
                
            case 'c': // Alternate config file
                conf_file=(char *)malloc(sizeof(optarg));
                strcpy(conf_file,optarg);
                break;
                
            case 'p': // Alternate listening port
                port = atoi(optarg);
                break;
                
            case 't': // test mode, no Lutron connection
                test_mode = TRUE;
                if(debug) printf("test mode\n");
                break;
                
            case 'k': // test mode, no Lutron connection
                kill_flag = TRUE;
                if(debug) printf("kill mode\n");
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
    
    if(debug)printf("command ops parsed\n");
    if(debug)printf("logfile: %s\n",admin.log_file);

    
    // create the worker threads
    thread_error = pthread_create(&lutron_tid, NULL, &lutron_connection, NULL);
    if (thread_error != 0)
        fprintf(stderr,"Can't create Lutron thread :[%s]\n", strerror(thread_error));
    else
        if(debug) printf("Thread created successfully\n");
    
    thread_error = pthread_create(&client_tid, NULL, &client_listen, NULL);
    if (thread_error != 0)
        fprintf(stderr,"Can't create Client listen thread :[%s]\n", strerror(thread_error));
    else
        if(debug) printf("Thread created successfully\n");
    
    if (testRoot()==EXIT_SUCCESS){
        if(debug)printf("root tested OK\n");
    }
    
    bool done = false;
    while( ! done ){
        usleep(20000);
        tlock.lock();
        if(tcount == 0) done = true;
        tlock.unlock();
    }
    
    std::cout << "Final tcount =" << tcount << "\n";
    
    exit(0);
}

