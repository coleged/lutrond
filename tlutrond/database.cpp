//
//  database.cpp
//  tlutrond
//
//  Created by Ed Cole on 22/04/2018.
//  Copyright © 2018 Ed Cole. All rights reserved.
//

#include "lutrond.h"
#include "externals.h"

//*************** dump_db()
void
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
        logMessage("Unable to open database file");
    }
    
    for(i=1;i<NO_OF_DEVICES;i++){
        if ((device[i].type != 0) && (device[i].state != NULL)){
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

