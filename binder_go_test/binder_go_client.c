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
    

struct binder_client_info {
    int binder_fd;
    void *map_address;
    struct content_sendto_user content;    
    void *output[CLIENT_SERVER_MAX];
};

static struct binder_client_info  binder_info;


struct rpc_node *cacu_next_client_node(const struct content_sendto_user *content, const void *map_address, const struct rpc_node *node)
{
    return cacu_node_address(content, map_address, node->next_client_req);
}

uint binder_init()
{
    uint ret;

    binder_info.binder_fd = open(BINDER_DRIVER,O_RDWR);
    if (binder_info.binder_fd < 0) {
        printf("open file error \n");
        return BIND_RET_ERROR;
    }

    ret = read_content(binder_info.binder_fd, &binder_info.content);
    if (ret == BIND_RET_ERROR) {
        printf("read address is %d;\n", binder_info.content.map_size);
        return BIND_RET_ERROR;
    }

    binder_info.map_address = mmap(NULL, binder_info.content.map_size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_LOCKED, binder_info.binder_fd, 0);
    if (binder_info.map_address == MAP_FAILED) {
        printf("map error %d;\n", binder_info.content.map_size);
        return BIND_RET_ERROR;
    }

    return BIND_RET_SUCCESS;
}

void binder_uninit()
{
    munmap(binder_info.map_address, binder_info.content.map_size);
    close(binder_info.binder_fd);
}

/*
    将输入参数拷贝到业务节点占用的共享内存
*/
uint set_input_para(struct rpc_para *para, uint cnt, uint start_node_id)
{
    uint i = 0;
    struct rpc_node *node = NULL;
    void *input;
    void *output;
    uint node_id = start_node_id;
    struct content_sendto_user *content = &binder_info.content;
    void *map_address= binder_info.map_address;

    while (i < cnt) {
        node = cacu_node_address(content, map_address, node_id);
        if (node == NULL) break;

        if (para[i].input_len > 0 )  {
            input = cacu_data_address(content, map_address, node->input_data);
            memcpy(input, para[i].input, para[i].input_len);
        }

        printf("node id is %d, input str is %s \n", node->node_id, (char *)input);
        output = cacu_data_address(content, map_address, node->output_data);
        binder_info.output[i] = output;
        node_id = node->next_client_req;
        i++;
    } 

    if (i != cnt) return BIND_RET_ERROR;
    return BIND_RET_SUCCESS;
}

/*
组装需要传递到内核态的数据，拷贝到per cpu地址
*/
uint make_percpu(const void *map_address, struct rpc_para *para, uint cnt)
{
    uint ret;
    uint i;
    struct bind_percpu percpu;
    struct bind_percpu *bind_percpu;

    memset(&percpu, 0 , sizeof(percpu));

    ret = read_content(binder_info.binder_fd, &binder_info.content);
    if (ret == BIND_RET_ERROR) {
        printf("read address is %d;\n", binder_info.content.map_size);
        return BIND_RET_ERROR;
    }

    bind_percpu = cacu_percpu_address(&binder_info.content, map_address);

    percpu.cpuid = binder_info.content.cpu_id;
    percpu.role = RPC_ROLE_CLIENT;
    percpu.pid = getpid();
    percpu.client_need_server_num = cnt;
    for(i = 0; i < cnt && i < CLIENT_SERVER_MAX; i++) {
        percpu.server_need_start[i].rpc_id = para[i].rpc_id;
        percpu.server_need_start[i].input_size = para[i].input_len;
        percpu.server_need_start[i].output_size = para[i].output_len;
    }

    memcpy(bind_percpu, &percpu, sizeof(percpu));

    return BIND_RET_SUCCESS;
}

void print_input(struct rpc_para *para, uint cnt)
{
    uint i = 0;
    for(;i < cnt; i++) {
        printf("input rpc_id:%d, input str:%s \n", para[i].rpc_id, (char *)para[i].input);
    }
}

void print_output(struct rpc_para *para, uint cnt)
{
    uint i = 0;
    for(;i < cnt; i++) {
        memcpy(para[i].output, binder_info.output[i], para[i].output_len);
        printf("input rpc_id:%d, output str:%s \n", para[i].rpc_id, (char *)para[i].output);
    }
}

/*
向内核注册客户端的需求
*/
uint reg_req(struct rpc_para *rpc_para, uint cnt)
{
    uint node_id;
    uint ret;
    int fd = binder_info.binder_fd;
    void *map_address = binder_info.map_address;

    if (cnt > CLIENT_SERVER_MAX) {
        return BIND_RET_ERROR;
    }

    do {
        ret = make_percpu(map_address, rpc_para, cnt);   //组装需要传递到内核态的数据，拷贝到per cpu地址

        if (ret != BIND_RET_SUCCESS) {
            usleep(100);
            continue;
        }

        ret = ioctl(fd, RPC_REQUEST_CMD, &node_id);
        printf("printf node id  %d\n",node_id);
    } while (ret != BIND_RET_SUCCESS);

    ret = set_input_para(rpc_para, cnt, node_id);
    if (ret != BIND_RET_SUCCESS) {
        printf("data error\n");
        return BIND_RET_ERROR;
    }

    print_input(rpc_para, cnt);

    ret = ioctl(fd, RPC_CLIENT_WAIT, NULL);
    if (ret != BIND_RET_SUCCESS) {
        return BIND_RET_ERROR;
    }

    print_output(rpc_para, cnt);

    ret = ioctl(fd, RPC_CLIENT_COMPLETE, NULL);   //tell kernel task completed , and free kernel resources
    if (ret != BIND_RET_SUCCESS) {
        return BIND_RET_ERROR;
    }

    return BIND_RET_SUCCESS;
}

uint call_binder_go(struct rpc_para *rpc_para, uint cnt)
{
    uint ret;

    ret = binder_init();
    if (ret != BIND_RET_SUCCESS) {
        printf("binder init error\n");
        return BIND_RET_ERROR;
    }
    ret = reg_req(rpc_para, cnt);
    if (ret != BIND_RET_SUCCESS) {
        printf("binder reg error\n");
        return BIND_RET_ERROR;
    }

    binder_uninit();

    return BIND_RET_SUCCESS;
}

