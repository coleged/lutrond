//
//  externals.h
//  
//
//  Created by Ed Cole on 19/04/2018.
//

#ifndef externals_h
#define externals_h

// globals for linkage across compiled object files

extern MessageQueue_t *mq;   // pointer to the inter-thread message queue
extern std::mutex tlock;
extern int tcount;
extern bool debug;
extern lut_dev_t device[NO_OF_DEVICES]; // database of Lutron devices
extern client_t listener;
extern lutron_t lutron;
extern daemon_t admin;
extern flags_t flag;
extern pid_t telnet_pid;
extern pthread_t lutron_tid,client_tid,lutron_tid2;                      // Thread IDs

#endif /* externals_h */
