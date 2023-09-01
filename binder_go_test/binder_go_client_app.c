#include <sys/mman.h>
#include <sys/ioctl.h>
#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "stdbool.h"

#include "binder_app.h"

#define BINDER_DRIVER "/dev/binder_pro"


void make_rpc_para(struct rpc_para *para, void **output, uint cnt)
{
    if (cnt > 5 || cnt ==0) {
        printf("bad cnt num %d\n", cnt);
        return;
    }
    para[0].rpc_id = 10013;
    para[0].input = "hello_world-10013";
    para[0].input_len = 100;
    para[0].output = output[0];
    para[0].output_len = 100;

    para[1].rpc_id = 10016;
    para[1].input = "hello_world-10016";
    para[1].input_len = 200;
    para[1].output = output[1];
    para[1].output_len = 200;

    para[2].rpc_id = 10023;
    para[2].input = "hello_world-10023";
    para[2].input_len = 300;
    para[2].output = output[2];
    para[2].output_len = 300;

    para[3].rpc_id = 10123;
    para[3].input = "hello_world-10123";
    para[3].input_len = 300;
    para[3].output = output[3];
    para[3].output_len = 300;

    para[4].rpc_id = 10133;
    para[4].input = "hello_world-10133";
    para[4].input_len = 300;
    para[4].output = output[4];
    para[4].output_len = 300;
}

int main(void) {
    uint ret;
    struct rpc_para para[5];
    char *output[5];
    uint i;
    for(i = 0; i < 5; i++) {
        output[i] = (char *)malloc(300);
    }
    make_rpc_para(para, (void **)output, 5);
    ret = call_binder_go(para,  5);
    if (ret != BIND_RET_SUCCESS) {
        return  BIND_RET_ERROR;
    }
    return 0;
}