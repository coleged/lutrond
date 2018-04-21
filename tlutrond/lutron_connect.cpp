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

/**************  TEST Lutron connection thread

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

************ END Lutron connection thread  */

void* lutron_connection(void *arg){
    pid_t pid;
    fd_set read_fd;
    fd_set write_fd;
    fd_set except_fd;
    std::string str;
    
    //Establish Lutron connection
    if(flag.debug) printf("connecting to Lutron\n");
    logMessage("Esablishing Luton connect");
    flag.connected = false;
    pid = forkpty(&lutron.fd, NULL, NULL, NULL);
    if (pid < 0) { // forking error
        // TODO error("Forking error");
        fprintf(stderr,"Forking error\n");
        exit(EXIT_FAILURE);
    }//if
    if (pid == 0) {// child
        if(!flag.test){
            if(flag.debug)printf("fork OK\n");
            const char *args[] = {"telnet",lutron.host,lutron.lport,NULL};
            execvp(TELNET_PROG, (char **)args);
        }else{
            printf("CHILD running in test mode\n");
            // TODO not sure this is working
            char input[256];
            bzero(input,256);
            while(true){
                if(scanf("%s\n",input)>0){
                    printf("+%s\n",input);
                }
            }//while
            exit(EXIT_SUCCESS);
        }//else
    }// if child
    //*************************************************END CHILD
    
    // Parent
    
    // set up lutron connection parameters.
    // no local echo
    struct termios tios;
    tcgetattr(lutron.fd, &tios);
    // unset echo and echo-newline bits in local modes
    tios.c_lflag &= ~(ECHO | ECHONL);
    tcsetattr(lutron.fd, TCSAFLUSH, &tios);
    //*************************************************END LUTRON EST
    
    
    // main server loop
    
    while(TRUE){
        if (!flag.test){
            if(flag.debug) printf("Lutron FD=%i",lutron.fd);
            while( !flag.connected){ // logon to lutron
                FD_ZERO(&read_fd);
                FD_ZERO(&write_fd);
                FD_ZERO(&except_fd);
                FD_SET(lutron.fd, &read_fd);
                select(lutron.fd+1, &read_fd, &write_fd, &except_fd, NULL);
                if (FD_ISSET(lutron.fd, &read_fd)){
                    if ((l_bytes=read(lutron.fd, &lutron_buff, lutron_buff_sz)) != -1){
                        if (flag.debug) write(STDOUT_FILENO, &lutron_buff, l_bytes);
                        if (strncmp(lutron_buff,"login: ",7) == 0){
                            write(lutron.fd,lutron.user,6);
                            write(lutron.fd,"\n",1);
                        }//if login
                        if (strncmp(lutron_buff,"password: ",10) == 0){
                            write(lutron.fd,lutron.password,11);
                            write(lutron.fd,"\n\n",2);
                        }//if password
                        if (strncmp(lutron_buff,"QNET> ",6) == 0){
                            if(flag.debug) printf("connected\n");
                            logMessage("Connected to Lutron");
                            flag.connected = true;
                        }//if QNET
                    }//If read
                }//if FD_ISSET
            }// While not_connected
        }else{// flag.test
            printf("PARENT:running in test mode - no Lutron connection\n");
            printf("PARENT:THIS ISN'T IMPLIMENTED YET BYE\n");
            exit(EXIT_FAILURE);
        }//if (else)  flag.test
        while(flag.connected){
            
            if(debug) printf("Connected to Lutron\n");
            
            pthread_mutex_lock(&mq->mu_queue);       // lock queue
            if(!mq->msg_queue.empty())               // if queue not empty
            {
                str = mq->msg_queue.front();         // read string at front
                //  of queue
                mq->msg_queue.pop();                 // and pop it
                // send command to lutron
                
                //TODO ******** THIS IS AS FAR AS I'VE GOT SO FAR
                
                std::cout << tcount << str << "\n";
            }// if !empty()
            pthread_mutex_unlock(&mq->mu_queue); // unlock queue
            pthread_cond_signal(&mq->cond);      // sig any blocked threads
            FD_ZERO(&read_fd);
            FD_ZERO(&write_fd);
            FD_ZERO(&except_fd);
            FD_SET(lutron.fd, &read_fd);
            FD_SET(lutron.fd, &read_fd);
            select(lutron.fd+1, &read_fd, &write_fd, &except_fd, NULL);
            if(FD_ISSET(lutron.fd, &read_fd)){
                    l_bytes=read(lutron.fd,&lutron_buff,lutron_buff_sz);
                    //parse_response("L3>>",lutron_buff);
                    if(debug) printf("L3>> %s\n",lutron_buff);
                    if(debug) printf("Done reading lutron(3)\n");
            }//if ISSET lutron
        }//while flag.connected
    }//while TRUE

    return((void *)EXIT_FAILURE);
    
}
