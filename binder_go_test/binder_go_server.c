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

struct binder_server_info {
    int fd;
    struct content_sendto_user content;
    void *map_address;
    struct binder_rpc_register *rpc_calls;
    uint rpc_cnt ;
};

static struct  binder_server_info binder_server_info;

uint binder_server_init(struct binder_rpc_register *rpc_calls, uint cnt )
{
    uint ret;
    int flags;
    int fd;
    binder_server_info.fd = open(BINDER_DRIVER,O_RDWR);
    if (binder_server_info.fd <= -1) {
        printf("open file error \n");
        return BIND_RET_ERROR;
    }
    ret = read_content(binder_server_info.fd, &binder_server_info.content);
    if (ret == BIND_RET_ERROR) {
        printf("read address is %d;\n", binder_server_info.content.map_size);
        return BIND_RET_ERROR;
    }

    flags = MAP_SHARED | MAP_LOCKED;
    fd =  binder_server_info.fd;
    binder_server_info.map_address = mmap(NULL, binder_server_info.content.map_size, PROT_READ|PROT_WRITE, flags, fd, 0);
    if (binder_server_info.map_address == MAP_FAILED) {
        printf("map error %d;\n", binder_server_info.content.map_size);
        return BIND_RET_ERROR;
    }

    binder_server_info.rpc_calls = rpc_calls;
    
    binder_server_info.rpc_cnt = cnt;

    return BIND_RET_SUCCESS;
}

void binder_server_uinit() {
    close(binder_server_info.fd);
    munmap(binder_server_info.map_address, binder_server_info.content.map_size);
}

uint register_rpc(struct binder_rpc_register *rpc_calls, uint cnt) {
    int ret;
    int fd = binder_server_info.fd;
    for (int i = 0; i <  cnt; i++) {
        ret = ioctl(fd, RB_TREE_INSERT_CMD, &rpc_calls[i].rpc_id);
        if (ret == -1) {
            printf("rpc id: %d register error\n", rpc_calls[i].rpc_id);
        }
    }
    return 0;
}

uint find_rpc(int fd, uint rpc_id)
{
    int ret;
    uint rpc_code = rpc_id;
    ret = ioctl(fd, RB_TREE_SEARCH_CMD, &rpc_code);
    if (ret == -1) {
        printf("rpc id: %d search error\n", rpc_code);
    }
    return BIND_RET_ERROR;
}


uint rpc_calls_func(struct rpc_node *node, struct content_sendto_user *content, void *map_address)
{
    struct rpc_para rpc_para;
    void *output;
    uint i;

    printf("node id is %d, input data block id is %d \n", node->node_id, node->input_data);
    printf("inpput id %d, output id %d \n", node->input_data, node->output_data);
    rpc_para.rpc_id = node->rpc_id;
    rpc_para.input = cacu_data_address(content,map_address, node->input_data);
    rpc_para.input_len = node->input_data_size;
    rpc_para.output = cacu_data_address(content,map_address, node->output_data);
    rpc_para.output_len = node->output_data_size;

    printf("inpput addr %p, output addr %p \n", rpc_para.input, rpc_para.output);

    for (i = 0; i < binder_server_info.rpc_cnt; i++) {
        if (binder_server_info.rpc_calls[i].rpc_id == node->rpc_id) {
            printf("node id is %d , input str is %s, rpc id is %d \n", node->node_id, (char *)rpc_para.input, node->rpc_id);
            output = binder_server_info.rpc_calls[i].rpc_call_fun(rpc_para.rpc_id, rpc_para.input, rpc_para.input_len); 
            if (output == NULL) {
                printf("rpc call error, rpc id :%d \n", rpc_para.rpc_id);
            } else {
                memcpy(rpc_para.output, output, rpc_para.output_len);
            }
        }
    }

    return BIND_INVLIED_ID;
}

struct rpc_node *find_node(struct content_sendto_user *content,void *map_address, uint node_id)
{
    struct rpc_node *node;
    node = (struct rpc_node *)(map_address + content->node_start_offset + content->node_size * node_id);
    return node;
}

uint rpc_call_onebyone(void *map_address, uint node_id) {
    struct rpc_node *node = NULL;
    uint node_find = node_id;
    struct content_sendto_user *content = NULL;
    uint ret;

    ret = read_content(binder_server_info.fd, &binder_server_info.content);
    if (ret == BIND_RET_ERROR) {
        printf("read address is %d;\n", binder_server_info.content.map_size);
        return BIND_RET_ERROR;
    }

    content = &binder_server_info.content;

    while(node_find != BIND_INVLIED_ID) {
        node = find_node(content,map_address, node_find);
        printf("print server === node id : %d node addr %p \n", node_find, node);
        rpc_calls_func(node, content, map_address);
        node->rpc_status = RPC_STATUS_SERVER_OVER;
        node_find = node->next_service_req;
        printf("print server node id : %d \n", node_find);
    };

    return BIND_RET_SUCCESS;
}

int check_rpc() {
    int ret;
    int node_id;
    int fd = binder_server_info.fd;
    void *map_address = binder_server_info.map_address;

    while(true) {
        ret = ioctl(fd, RPC_SERVER_WAIT, &node_id);
        printf("get node id from kernel %d\n", node_id);
        if (ret) {
            printf("data error\n");
            usleep(200);
            continue;
        }
        ioctl(fd, RPC_CLIENT_DSP, &ret);
        ioctl(fd, RPC_SERVER_DSP, &ret);
        rpc_call_onebyone(map_address, node_id);
        ioctl(fd, RPC_SERVER_COMPLETE, &node_id);
    }
}

uint start_binder_server(struct binder_rpc_register *rpc_calls, uint cnt)
{
    uint ret;
    ret = binder_server_init(rpc_calls, cnt);
    if (ret != BIND_RET_SUCCESS) {
        return BIND_RET_ERROR;
    }

    register_rpc(rpc_calls, cnt);
    check_rpc();

    binder_server_uinit();
    return 0;
}

