//
//  conf_file.cpp
//  tlutrond
//
//  Created by Ed Cole on 20/04/2018.
//  Copyright © 2018 Ed Cole. All rights reserved.
//

#include "lutrond.h"
#include "externals.h"

//************   readConfFile
int
readConfFile( char *path)    // requires libconfig (libconfig.h & -lconfig)
{                            // some difficulty in getting xcode to use these
    int ivalue,i,j;
    static config_t cfg;
    config_setting_t *setting,*comp_setting;
    const char *sport;
    config_init(&cfg);
    char key[64];            // key token for config lookups
    const char *value;        // value token for config lookups
    const char *ptr;
    
    if(! config_read_file(&cfg, path )){
        fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
                config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        return(EXIT_FAILURE);
    }//if
    
    if(listener.port == 0){
        if(config_lookup_string(&cfg, "listen_port", &sport)){
            if(!flag.port) listener.port=atoi(sport); // if no -p switch on cmd line
        }//if
    }//if port
    // TODO - see these strange shenanigans via const char *ptr to get
    // string element of structure assigned to config batabase. There must be
    // a cleaner way.
    if(config_lookup_string(&cfg, "lutron_host",&ptr)){
        lutron.host = (char *)ptr;
        if(flag.debug) printf("Lutron Host = %s\n",lutron.host);
    }
    if(config_lookup_string(&cfg, "lutron_user",&ptr)){
        lutron.user = (char *)ptr;
        if(flag.debug) printf("Lutron User = %s\n",lutron.user);
    }
    if(config_lookup_string(&cfg, "lutron_password",&ptr)){
        lutron.password = (char *)ptr;
        if(flag.debug) printf("Lutron Password = %s\n",lutron.password);
    }
    if(config_lookup_string(&cfg, "log_file",&ptr)){
        admin.log_file = (char *)ptr;
        if(flag.debug) printf("Log file = %s\n",admin.log_file);
    }
    if(config_lookup_string(&cfg, "pid_file",&ptr)){
        admin.pid_file = (char *)ptr;
        if(flag.debug) printf("Pid file = %s\n",admin.pid_file);
    }
    if(config_lookup_string(&cfg, "db_file",&ptr)){
        admin.db_file = (char *)ptr;
        if(flag.debug) printf("Database file = %s\n",admin.db_file);
    }
    // Device names
    for(i=1;i<NO_OF_DEVICES;i++){
        sprintf(key,"device_%i",i);
        setting = config_lookup(&cfg, key);
        if (setting != NULL){
            /*Read the string*/
            if (config_setting_lookup_string(setting, "name", &value)){
                device[i].name=(char *)value;
            }else{
                device[i].name=NULL;
            } // if else
            if (config_setting_lookup_int(setting, "type", &ivalue)){
                device[i].type=ivalue;
            }else{
                device[i].type=0;
            }
            if (config_setting_lookup_string(setting, "location", &value)){
                device[i].location=(char *)value;
            }else{
                device[i].location=NULL;
            }
            // Component setting
            for(j=1;j<NO_OF_COMPS;++j){
                device[i].comp[j].empty=1;// start with no components
                sprintf(key,"comp_%i",j);
                comp_setting = config_setting_lookup(setting, key);
                if(comp_setting != NULL){
                    //****** COMP
                    if (config_setting_lookup_string(comp_setting, "comp", &value)){
                        device[i].comp[j].comp=(char *)value;
                        //********  INCREMENT EMPTY AS EACH COMP IS LOADED
                        device[i].comp[j].empty++; // component found
                    }else{
                        device[i].comp[j].comp=NULL;
                    }
                    //******  NUM
                    if (config_setting_lookup_int(comp_setting, "num", &ivalue)){
                        device[i].comp[j].num=ivalue;
                    }else{
                        device[i].comp[j].num=0;
                    }
                    //****** TYPE
                    if (config_setting_lookup_int(comp_setting, "type", &ivalue)){
                        device[i].comp[j].type=ivalue;
                    }else{
                        device[i].comp[j].type=0;
                    }
                    //******* NAME
                    if (config_setting_lookup_string(comp_setting, "name", &value)){
                        device[i].comp[j].name=(char *)value;
                    }else{
                        device[i].comp[j].name=NULL;
                    }
                    //******* VALUE
                    if (config_setting_lookup_int(comp_setting, "value", &ivalue)){
                        device[i].comp[j].value=ivalue;
                    }else{
                        device[i].comp[j].value=0;
                    }
                }//if
                // TODO
                // if empty == j we have too many components
                // probably an error in the config file
            }//for
        }// if setting
    }//for
    
    // we dont destroy the cfg, as to do so would free up the memory
    // where the above strings live
    //config_destroy(&cfg);
    return(EXIT_SUCCESS);
}//readConfFile
