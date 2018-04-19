//
//  lutron_connect.cpp
//  
//
//  Created by Ed Cole on 19/04/2018.
//

#include "lutrond.h"
#include "externals.h"

char lutron_buff[BUFFERSZ];
int lutron_buff_sz = sizeof(lutron_buff);
int l_bytes;

//**************  Lutron connection thread

void* lutron_connection(void *arg){
    
    std::string str;
    
    tlock.lock();
    ++tcount;
    tlock.unlock();
    if(debug) printf("client thread started\n");
    while(1)                              // queue reading (popping) loop
    {
        pthread_mutex_lock(&mq->mu_queue);       // lock queue
        if(!mq->msg_queue.empty())               // if queue not empty
        {
            str = mq->msg_queue.front();         // read string at front
            //  of queue
            mq->msg_queue.pop();                 // and pop it
            pthread_mutex_unlock(&mq->mu_queue); // unlock queue
            pthread_cond_signal(&mq->cond);      // sig any blocked threads
            if ( str.empty()){                   // if empty string break
                // loop we're done
                break; // while
            }
            std::cout << tcount << str << "\n";
        }
    }//while 1
    tlock.lock();
    --tcount;
    tlock.unlock();
    return(NULL);
    
}

//*************** END Lutron connection thread

