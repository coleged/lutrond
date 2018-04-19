/*
main.cpp    ->  tlutrond

Created by Ed Cole on 19/04/2018.
Copyright © 2018 Ed Cole. All rights reserved.

threaded version of lutrond

*/

#include <iostream>
#include <queue>
#include <unistd.h>
#include <mutex>

#define _DEBUG true

int tcount = 0;                     // thread counter - simple wait mechanism for now
                                    // OK for thread finnishing normally, but won't
                                    // work if thread dies abnornally
std::mutex tlock;                   // general purpose thread lock
                                    // TODO better thread monitor. All this does at present is
                                    // increment counter when thread starts, decrement when ends
                                    // main thread loops until count == 0

bool debug = _DEBUG;
struct MessageQueue                 // define a queue struct for strings
{
    std::queue<std::string> msg_queue;
    pthread_mutex_t mu_queue;
    pthread_cond_t cond;
};



MessageQueue queue;                 // create in instance of a queue
MessageQueue *mq = &queue;          // and a pointer to this queue

//**************  Lutron connection thread


char *getString(){     // uses a static index (i) to return
    // a number of C-strings each time it's called
    static int i = 0;
    static const char * strs[] = {  "hello world",
        "second message",
        "now added a third",
        "Last message on queue",
        "\0"};
    
    return((char *)strs[i++]);
    
}

int pushq(std::string arg) // pushes string onto queue
{
        pthread_mutex_lock(&mq->mu_queue);    // lock the queue via the pointer
        mq->msg_queue.push(arg);              // push the string onto queue
        pthread_mutex_unlock(&mq->mu_queue);  // unlock the queue
        pthread_cond_signal(&mq->cond);       // signal any thread waiting on
        return(EXIT_SUCCESS);
}

void pusher()
{
    std::string msg;        // C++ string
    
    while(1){
        msg = getString();
        pushq(msg);
        if( msg.empty()){
            break;
        }
    }
    
}

void* lutron_connection(void *arg){
    tlock.lock();
    ++tcount;
    tlock.unlock();
    if(debug) printf("lutron thread started\n");
    pusher();
    tlock.lock();
    --tcount;
    tlock.unlock();
    return(NULL);
}

//*************** END Lutron connection thread

//*************** Client connect thread

void* client_listen(void *arg){
    
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


//*************** END client connection thread


int main(int argc, const char * argv[]) {
    
    
    pthread_t lutron_tid,client_tid;                      // Thread IDs
    int thread_error;
    
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

