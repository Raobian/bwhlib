/* ************************************************************************
 *       Filename:  test_liblog.c
 *    Description:  
 *        Version:  1.0
 *        Created:  07/05/2017 04:28:42 PM
 *       Revision:  none
 *       Compiler:  gcc
 *         Author:  YOUR NAME (), 
 *        Company:  
 * ************************************************************************/

#include <stdio.h>
#include "liblog.h"

int main(int argc, char *argv[])
{
        int ret;
        ret = ylog_init(YLOG_FILE, "tmp.log");
        if (ret) 
                printf("ret %d\n", ret);

        DINFO("test a log\n");

        return 0;
}


