//
//  externals.h
//  
//
//  Created by Ed Cole on 19/04/2018.
//

#ifndef externals_h
#define externals_h

extern MessageQueue *mq;
extern std::mutex tlock;
extern int tcount;
extern bool debug;
extern lut_dev_t device[NO_OF_DEVICES]; // database of Lutron devices

#endif /* externals_h */
