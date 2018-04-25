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
 
 lutron_connect.cpp
 
 lutrond V4.0 April 2018
 
 lutron_connection() lutron_doit()
 
 ***********/


#include "lutrond.h"
#include "externals.h"
#define __SELECT_TIMEOUT 1

char lutron_buff[BUFFERSZ];
int lutron_buff_sz = sizeof(lutron_buff);
int l_bytes;

void* lutron_doit(void *);

//************* lutron_connection()
void* lutron_connection(void *arg){
    
    
    // int status;
    
    
    while( true ){
        // create lutron connection thread
        if(flag.debug) printf("lutron_connection: new connection\n");
        if( pthread_create(&lutron_tid2, NULL, &lutron_doit, NULL) != 0){
            fprintf(stderr,"lutron_connection: new connection\n");
            logMessage("lutron_connection: failed to create thread");
        };
        pthread_join (lutron_tid2, NULL); // wait for thread to die
        // not sure about the need for this wait and wonder it might block
        // while( waitpid(telnet_pid,&status, WNOHANG) > 0){}; // waits for child to die
        if(flag.debug) printf("lutron_connection: thread died\n");
    }
    
}//END************* lutron_connection()


//************* lutron_doit()
void* lutron_doit(void *arg){
    pid_t pid;
    fd_set read_fd;
    fd_set write_fd;
    fd_set except_fd;
    std::string str;
    struct timeval tv = {__SELECT_TIMEOUT, 0};
    
    
    if(flag.dump){
        dump_db();
        flag.dump=false;
    }// if
    
    
    if( !flag.test){
        //Establish Lutron connection
        if(flag.debug) printf("connecting to Lutron\n");
        logMessage("Esablishing Luton connect");
        flag.connected = false;
        if (lutron.fd > 0) close(lutron.fd); // close the existing file descriptor if open
        pid = forkpty(&lutron.fd, NULL, NULL, NULL);
        if (pid < 0) { // forking error
            // TODO error("Forking error");
            fprintf(stderr,"Forking error\n");
            exit(EXIT_FAILURE);
        }//if
        
        if (pid == 0) {// child
            
                if(flag.debug)printf("fork OK\n");
                const char *args[] = {"telnet",lutron.host,lutron.lport,NULL};
                execvp(TELNET_PROG, (char **)args);
            
        }// if child
        //*************************************************END CHILD
        
        // Parent
        
        // set up lutron connection parameters.
        // no local echo
        telnet_pid = pid;
        if(flag.debug)printf("Telnet pid is %i\n",pid);
        struct termios tios;
        tcgetattr(lutron.fd, &tios);
        // unset echo and echo-newline bits in local modes
        tios.c_lflag &= ~(ECHO | ECHONL);
        tcsetattr(lutron.fd, TCSAFLUSH, &tios);
        //*************************************************END LUTRON EST
        
                if(flag.debug) printf("Lutron FD=%i",lutron.fd);
        
                // Logon loop
                while( !flag.connected){ // logon to lutron
                    FD_ZERO(&read_fd);
                    FD_ZERO(&write_fd);
                    FD_ZERO(&except_fd);
                    FD_SET(lutron.fd, &read_fd);
                    select(lutron.fd+1, &read_fd, &write_fd, &except_fd, &tv);
                    if (FD_ISSET(lutron.fd, &read_fd)){
                        // TODO: Might be better to use Readline (readline.cpp) here
                        if ((l_bytes=(int)read(lutron.fd, &lutron_buff, lutron_buff_sz)) != -1){
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
                // NOW LOGGED ON
        
            if(flag.debug) printf("Connected to Lutron\n");
            while(flag.connected){
                
                pthread_mutex_lock(&mq->mu_queue);       // lock queue
                if(!mq->msg_queue.empty())               // if queue not empty
                {
                    str = mq->msg_queue.front();         // read string at front
                    //  of queue
                    mq->msg_queue.pop();                 // and pop it
                    // send command to lutron
                    write(lutron.fd,str.c_str(),strlen(str.c_str()));
                    write(lutron.fd,"\r\n",2);
                }// if !empty()
                pthread_mutex_unlock(&mq->mu_queue); // unlock queue
                pthread_cond_signal(&mq->cond);      // sig any blocked threads
                FD_ZERO(&read_fd);
                FD_ZERO(&write_fd);
                FD_ZERO(&except_fd);
                FD_SET(lutron.fd, &read_fd);
                FD_SET(lutron.fd, &read_fd);
                select(lutron.fd+1, &read_fd, &write_fd, &except_fd, &tv);
                if(FD_ISSET(lutron.fd, &read_fd)){
                    bzero(lutron_buff,lutron_buff_sz);
                    l_bytes=(int)read(lutron.fd,&lutron_buff,lutron_buff_sz);
                    parse_response((char *)"L3>>",lutron_buff);
                    if(flag.debug) printf("L3>> %s\n",lutron_buff);
                    // if(flag.debug) printf("Done reading lutron(3)\n");
                }//if ISSET lutron
            }//while flag.connected
        
    }else{ // test mode code here ......
        printf("No Lutron Connection: TEST MODE\n");
        while(true){
            
            if(!mq->msg_queue.empty())               // if queue not empty
            {
                pthread_mutex_lock(&mq->mu_queue);       // lock queue
                str = mq->msg_queue.front();        // read string off queue
                mq->msg_queue.pop();                // and pop it
                pthread_mutex_unlock(&mq->mu_queue);    // unlock queue
                pthread_cond_signal(&mq->cond);         // sig any blocked threads
                printf(">>>>>>>>>>>>>>%s",str.c_str());         // print it
            }// if !empty()
            usleep(10000);
        }
    }
    if(flag.debug)printf("lutron_doit: thread terminating\n");
    pthread_exit(NULL);

}//END************* lutron_doit()
