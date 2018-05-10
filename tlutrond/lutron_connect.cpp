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
 
 lutron_connection() lutron_doit() readPipe()
 
 ***********/


#include "lutrond.h"
#include "externals.h"
#define __SELECT_TIMEOUT 120

char lutron_buff[BUFFERSZ];
int lutron_buff_sz = sizeof(lutron_buff);
int l_bytes;

void* lutron_doit(void *);


//************* readPipe()
int readPipe(char *buffer,int buff_sz){
    
    int nread;
    
    if((nread = (int)read(pfd[0],buffer,buff_sz)) < 0){
        return(EXIT_FAILURE);
    }
    return (nread);
    
}
//END*************readPipe()


//************* lutron_connection()
void* lutron_connection(void *arg){
    
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    pthread_sigmask(SIG_UNBLOCK, &set, NULL);
    
    // perpetually respawn Luton Connection threads.
    
    while( true ){
        // create lutron connection thread
        if(flag.debug) fprintf(stderr,"lutron_connection: new connection\n");
        if( pthread_create(&lutron_tid2, NULL, &lutron_doit, NULL) != 0){
            logMessage("lutron_connection: failed to create thread");
            error("Lutron thread");
        };
        pthread_join (lutron_tid2, NULL); // wait for thread to die
        if(flag.debug) fprintf(stderr,"lutron_connection: thread died\n");
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
    char p_buffer[PIPE_BUFFER];
    int p_buffer_sz = PIPE_BUFFER;
    int l_bytes,p_bytes;
    
    
    if(flag.dump){
        dump_db();
        flag.dump=false;
    }// if
    
    
    if( !flag.test){
        //Establish Lutron connection
        logMessage("Esablishing Luton connect");
        flag.connected = false;
        if (lutron.fd > 0) close(lutron.fd); // close the existing file descriptor if open
        pid = forkpty(&lutron.fd, NULL, NULL, NULL);
        if (pid < 0) { // forking error
            // TODO error("Forking error");
            fprintf(stderr,"Forking error\n");
            error("Forking");
        }//if
        
        if (pid == 0) {// child
            
#ifdef LINUX
                prctl(PR_SET_PDEATHSIG, SIGTERM); // send child SIGTERM when parent dies
#endif
                // we're also sending SIGKILL to telnet as part of process SIGTERM
                // handler, so the above Linux only feature might be unnecessary anyway
            
            
                if(flag.debug)fprintf(stderr,"fork OK\n");
                const char *args[] = {"telnet",lutron.host,lutron.lport,NULL};
                execvp(TELNET_PROG, (char **)args);
                // if we get here, exec must have failed
                error("exec");
            
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
        
                if(flag.debug) fprintf(stderr,"Lutron FD=%i",lutron.fd);
        
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
        
            if(flag.debug) fprintf(stderr,"Connected to Lutron\n");
        
            // monitor pipe and lutron
            while(flag.connected){
                FD_ZERO(&read_fd);
                FD_ZERO(&write_fd);
                FD_ZERO(&except_fd);
                FD_SET(pfd[0], &read_fd);
                FD_SET(lutron.fd, &read_fd);
                select(lutron.fd+1, &read_fd, nullptr, nullptr, nullptr);
                    if(FD_ISSET(lutron.fd, &read_fd)){
                        bzero(lutron_buff,lutron_buff_sz);
                        l_bytes=(int)read(lutron.fd,&lutron_buff,lutron_buff_sz);
                        if(flag.debug)printf("\n");
                        parse_response((char *)"LC>>",lutron_buff);
                    }//if ISSET lutron
                    if(FD_ISSET(pfd[0], &read_fd)){
                        p_bytes = readPipe(p_buffer,p_buffer_sz);
                        write(lutron.fd,p_buffer,p_bytes);
                        write(lutron.fd,"\r\n",2);
                    }
            }//while flag.connected
        
    }else{ // test mode code here ......
        printf("No Lutron Connection: TEST MODE\n");
        while(true){
            FD_ZERO(&read_fd);
            FD_ZERO(&write_fd);
            FD_ZERO(&except_fd);
            FD_SET(pfd[0], &read_fd);
            select(pfd[0]+1, &read_fd, nullptr, nullptr, nullptr);
                if(FD_ISSET(pfd[0], &read_fd)){
                    p_bytes = readPipe(p_buffer,p_buffer_sz);
                    printf("%s",p_buffer);
                }
        }
    }
    if(flag.debug)fprintf(stderr,"lutron_doit: thread terminating\n");
    pthread_exit(NULL);

}//END************* lutron_doit()
