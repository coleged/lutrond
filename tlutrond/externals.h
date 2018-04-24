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
 
 externals.h
 
 lutrond V4.0 April 2018
 
 ***********/


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
