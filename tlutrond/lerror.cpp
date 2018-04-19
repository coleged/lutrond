/********

lerror.c

********/
#include "lutrond.h"
#include "externals.h"


char *lerror(int number){

static const char *msg[] = {
                "",
                "Parameter count mismatch",     //1
                "Object does not exist",        //2
                "Invalid action number",        //3
                "Parameter data out of range",  //4
                "Parameter data malformed",     //5
                "Unsupported Command"           //6
             };

return((char *)msg[number]);


} // lerror

void printLerrors(){
    int i;
    // TODO left here to remind me that we need to parse Lutron error messages
    if(debug){ // prints lutron error table
        for(i=1;i<7;i++){
            printf("%s\n",(const char *)lerror(i));
        }//for
    }// if debug
}
