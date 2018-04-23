//
//  client_connect.cpp
//  
//
//  Created by Ed Cole on 19/04/2018.
//

#include "lutrond.h"
#include "externals.h"

int bytes_in;
socklen_t socklen, cli_socklen;
char soc_buffer[BUFFERSZ];
struct sockaddr_in serv_addr, act_addr, cli_addr;

int client_connect=0;


//*************** Client connect thread

char *getString(){     // uses a static index (i) to return
    // a number of C-strings each time it's called
    static int i = 0;
    static const char * strs[] = {  "#OUTPUT,51,1,0.00",
        "#OUTPUT,52,1,0.00",
        "#OUTPUT,51,1,100.00",
        "#OUTPUT,52,1,100.00",
        "\0"};
    
    return((char *)strs[i++]);
    
}

int pushq(std::string arg) // pushes string onto queue
{
    if(flag.debug)printf("pushing %s\n",arg.c_str());
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
        //if(flag.debug)printf("pushing %s\n",msg.c_str());
    }
    
}

void* client_listen(void *arg){
    
    int optval=1;
    std::string msg;
    fd_set read_fd;
    fd_set write_fd;
    fd_set except_fd;
    
 
    // Establish Listening socket
    if(flag.debug) logMessage("Port number %i",listener.port);
    listener.sockfd = socket(AF_INET, SOCK_STREAM, 0); // create socket
    fcntl(listener.sockfd, F_SETFD, FD_CLOEXEC);
    if (listener.sockfd < 0){
        //error("ERROR opening socket");
        if(flag.debug)fprintf(stderr,"cant open listening socket\n");
    }
    if (setsockopt(listener.sockfd,
                   SOL_SOCKET,
                   SO_REUSEADDR,
                   &optval,             // {=1}
                   sizeof(int)) < 0){
        //error("setsockopt(SO_REUSEADDR) failed");
        if(flag.debug)fprintf(stderr,"cant SO_REUSEADDR socket\n");
    }//if setsocketopt
    socklen = sizeof(serv_addr);
    bzero((char *) &serv_addr, socklen);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(listener.port);
    if(flag.debug){
        logMessage("bind socket");
        printf("bind socket(FD = %i)\n",listener.sockfd);
    }// if debug
    if (bind(listener.sockfd,(struct sockaddr *) &serv_addr, socklen) < 0) {
        //error("ERROR on binding");
        if(flag.debug)fprintf(stderr,"binding error\n");
    }//if
    if(flag.debug) logMessage("listen");
    listen(listener.sockfd,SOC_BL_QUEUE);

    while(true){
        if (!flag.test){  // probably not needed as we always want listener loop
            FD_ZERO(&read_fd);
            FD_ZERO(&write_fd);
            FD_ZERO(&except_fd);
            FD_SET(listener.sockfd, &read_fd);
            select(listener.sockfd+1, &read_fd, &write_fd, &except_fd, NULL);
            if(FD_ISSET(listener.sockfd, &read_fd)){ // client connect attempt
                listener.actsockfd = accept(listener.sockfd,
                                            (struct sockaddr *) &act_addr,&socklen);
                if (listener.actsockfd < 0){
                    // ISSUE. A failed client accept crashes the server
                    // error("ERROR on accept");
                    printf("failed to accept connection\n");
                }else{
                    listener.connected = true;
                    if(flag.debug) printf("Accept connection (active socket FD= %i)\n",
                                     listener.actsockfd);
                }//else
                cli_socklen = sizeof(cli_addr);
                getpeername(listener.actsockfd,
                            (struct sockaddr *) &cli_addr,
                            &cli_socklen);
                if(flag.debug){
                    printf("Connection received\n");
                }// if
                
                syslog(SYSLOG_OPT,"Connection from %s",inet_ntoa(cli_addr.sin_addr));
                logMessage("Connection from %s ==========",inet_ntoa(cli_addr.sin_addr));
            }// if FD_ISSET(read) on listener.sockfd
            
            while( listener.connected ){
                FD_ZERO(&read_fd);
                FD_ZERO(&write_fd);
                FD_ZERO(&except_fd);
                FD_SET(listener.actsockfd, &read_fd);
                if(select(listener.actsockfd+1, &read_fd, &write_fd, &except_fd, NULL)>0){
                    if (FD_ISSET(listener.actsockfd, &read_fd)){
                        // we process all that the client has to say
                        if(flag.debug)printf("input from client\n");
                        bzero(&soc_buffer,BUFFERSZ);
                        while((bytes_in = (int)Readline(listener.actsockfd,
                                                   soc_buffer,BUFFERSZ)) > 0 ){
                            // if(flag.debug) printf("line read %s\n",soc_buffer);
                            write(listener.actsockfd,"thanks\n",7); // client waits for ack
                            // do we need to do this. Lutron echos the command and it gets
                            // parsed then
                            //parse_response("C1<<",soc_buffer);
                            if(flag.debug || flag.test) printf("%sreceived\n",soc_buffer);
                            if(!flag.test){
                            
                                // write it to queue
                                //write(lutron,soc_buffer,bytes_in+1);
                                msg = soc_buffer;
                                pushq(msg);
                                
                             }else{ //test mode
                                
                             }// else test mode
                             if(flag.debug) printf("C1<< %s\n",soc_buffer);
                             //parse_response("C1<<",soc_buffer);
                           } // while read
                           if(flag.debug) printf("Done reading client\n");
                           close(listener.actsockfd);
                           listener.connected=false;
                        }//if FD_SET
                    }// select
            }//while listener.connected
            
        }else{// flag.test==true
            // not sure there is any case here
        }// if/else flag.test
    }// while true
 

}// client_listen()

  

//*************** END client connection thread

