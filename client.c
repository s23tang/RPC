/*
 * client.c
 *
 * This file is the client program,
 * which prepares the arguments, calls "rpcCall", and checks the returns.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rpc.h"

#define CHAR_ARRAY_LENGTH 100

int main() {
    
    
    /* prepare the arguments for f3 */
    long a3[11] = {11, 109, 107, 105, 103, 101, 102, 104, 106, 108, 110};
    int count3 = 1;
    int argTypes3[count3 + 1];
    void **args3;
    
    argTypes3[0] = (1 << ARG_OUTPUT) | (1 << ARG_INPUT) | (ARG_LONG << 16) | 11;
    argTypes3[1] = 0;
    
    args3 = (void **)malloc(count3 * sizeof(void *));
    args3[0] = (void *)a3;

    /* prepare the arguments for f3 */
    long a4[11] = {11, 109, 107, 105, 103, 101, 102, 104, 106, 108, 110};
    int count4 = 1;
    int argTypes4[count4 + 1];
    void **args4;
    
    argTypes4[0] = (1 << ARG_OUTPUT) | (1 << ARG_INPUT) | (ARG_LONG << 16) | 11;
    argTypes4[1] = 0;
    
    args4 = (void **)malloc(count4 * sizeof(void *));
    args4[0] = (void *)a4;

     /* prepare the arguments for f3 */
    long a5[11] = {11, 109, 107, 105, 103, 101, 102, 104, 106, 108, 110};
    int count5 = 1;
    int argTypes5[count5 + 1];
    void **args5;
    
    argTypes5[0] = (1 << ARG_OUTPUT) | (1 << ARG_INPUT) | (ARG_LONG << 16) | 11;
    argTypes5[1] = 0;
    
    args5 = (void **)malloc(count5 * sizeof(void *));
    args5[0] = (void *)a5;
    

    int s3 = rpcCacheCall("f3", argTypes3, args3);
    /* test the return of f3 */
    printf(
           "\nEXPECTED return of f3 is: 110 109 108 107 106 105 104 103 102 101 11\n"
           );
    
    if (s3 >= 0) {
        printf("ACTUAL return of f3 is: ");
        int i;
        for (i = 0; i < 11; i++) {
            printf(" %ld", *(((long *)args3[0]) + i));
        }
        printf("\n");
    }
    else {
        printf("Error: %d\n", s3);
    }

    int s4 = rpcCacheCall("f3", argTypes4, args4);
    /* test the return of f3 */
    printf(
           "\nEXPECTED return of f3 is: 110 109 108 107 106 105 104 103 102 101 11\n"
           );
    
    if (s4 >= 0) {
        printf("ACTUAL return of f3 is: ");
        int i;
        for (i = 0; i < 11; i++) {
            printf(" %ld", *(((long *)args4[0]) + i));
        }
        printf("\n");
    }
    else {
        printf("Error: %d\n", s4);
    }

    int s5 = rpcCacheCall("f3", argTypes5, args5);
    /* test the return of f3 */
    printf(
           "\nEXPECTED return of f3 is: 110 109 108 107 106 105 104 103 102 101 11\n"
           );
    
    if (s5 >= 0) {
        printf("ACTUAL return of f3 is: ");
        int i;
        for (i = 0; i < 11; i++) {
            printf(" %ld", *(((long *)args5[0]) + i));
        }
        printf("\n");
    }
    else {
        printf("Error: %d\n", s5);
    }


    /* end of client.c */
    return 0;
}