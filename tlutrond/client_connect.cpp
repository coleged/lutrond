//
//  client_connect.cpp
//  
//
//  Created by Ed Cole on 19/04/2018.
//

#include "lutrond.h"
#include "externals.h"

int sockfd, actsockfd; // file descriptors for socket
int bytes_in;
socklen_t socklen, cli_socklen;
char soc_buffer[BUFFERSZ];
struct sockaddr_in serv_addr, act_addr, cli_addr;

int client_connect=0;

//************   testRoot
int
testRoot(){
  /*
    int user;
    
    user=getuid();
    if(user != 0){
        fprintf(stderr,"Must be root to run this\n");
        error("Not root!");
    }//if
    return(EXIT_SUCCESS);
   */
    return(EXIT_SUCCESS);
    
}//testRoot

//*************** Client connect thread

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

void* client_listen(void *arg){
    
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




//*************** END client connection thread

