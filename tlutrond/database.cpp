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
 
 database.cpp
 
 lutrond V4.0 April 2018
 
 dump_db(), db_timestamp()
 
 ***********/

#include "lutrond.h"
#include "externals.h"

//*************** dump_sysvar()
int
dump_sysvar(){
    static FILE *svfp;
    
    mode_t m;
    m = umask(033);
    svfp = fopen(admin.sysvar_file, "w");
    umask(m);
    
    if (svfp == NULL){
        logMessage("dump_sysvar(): Unable to open database file %s",admin.sysvar_file);
        return(EXIT_FAILURE);
    }
    for(int i=1;i<NO_OF_DEVICES;i++){
        if (device[i].type == 50){
            fprintf(svfp,"%i\t=\t%s\t#%s:%s\n",i,
                                            device[i].state,
                                            device[i].name,
                                            device[i].location
                                            );
        }
    }//for i
    fclose(svfp);
    
    return (EXIT_SUCCESS);
    
}// dump_sysvar()

//*************** dump_db()
int
dump_db(){
    
    int i,j;
    char dbFilename[128];
    static FILE *dbfp;
    
    bzero(dbFilename,128);
    sprintf(dbFilename,"%s.%s",admin.db_file,db_timestamp());
    mode_t m;
    m = umask(033);
    dbfp = fopen(dbFilename, "w");
    umask(m);
    
    if (dbfp == NULL){
        logMessage("dump_db(): Unable to open database file %s",dbFilename);
        return(EXIT_FAILURE);
    }
    
    for(i=1;i<NO_OF_DEVICES;i++){
        if ((device[i].type != 0)){
            fprintf(dbfp,"device_%i = {\n",i);
            fprintf(dbfp,"\ttype = %i;\n",device[i].type);
            fprintf(dbfp,"\tlocation = \"%s\";\n",device[i].location);
            fprintf(dbfp,"\tname = \"%s\";\n",device[i].name);
            fprintf(dbfp,"\tstate = \"%s\";\n",device[i].state);
            for( j=1;j<NO_OF_COMPS;++j){
                if( device[i].comp[j].num > 0){
                    fprintf(dbfp,"\tcomp_%i = {\n",j);
                    fprintf(dbfp,"\t\tcomp = \"%s\";\n",device[i].comp[j].comp);
                    fprintf(dbfp,"\t\tnum = %i;\n",device[i].comp[j].num);
                    fprintf(dbfp,"\t\tname = \"%s\";\n",device[i].comp[j].name);
                    fprintf(dbfp,"\t\ttype = %i;\n",device[i].comp[j].type);
                    fprintf(dbfp,"\t\tvalue = %i;\n",device[i].comp[j].value);
                    fprintf(dbfp,"\t      };\n"); // close component
                    
                }//if
            }//for
            fprintf(dbfp,"\t};\n"); // close device
        }
    }//for i
    fclose(dbfp);
    return(EXIT_SUCCESS);
    
    
    
    
}//dump_db()


//************** timestamp()
char *db_timestamp(){
    
    static char stamp[14];   //yyyymmddhhmmss
    
    time_t now = time(NULL);
    struct tm *today = gmtime(&now);
    int year = today->tm_year + 1900;
    int month = today->tm_mon + 1;
    int day = today->tm_mday;
    int hour = today->tm_hour;
    if (today->tm_isdst == 0) ++hour;
    int min = today->tm_min;
    int sec = today->tm_sec;
    
    sprintf(stamp,"%i%02d%02d%02d%02d%02d",year,month,day,hour,min,sec);
    
    return(stamp);
    
}// timestamp
//*************** timestamp()

