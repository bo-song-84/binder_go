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

/*
struct bind_percpu{	
	//mutex_lock lock;  //主要是为了防止进程被抢占导致的数据异常，如果开启禁止抢占内核，这个锁就不需要了
	uint cpuid; 		//cpuid
	pid_t pid;			//当前使用进程的pid
	uint role;			//当前进程的角色  server、client or no rpc
	uint server_start_id;// 当前进程为server时，第一个需要服务的node节点id
	uint client_server_max; //一个客户端可以同时申请服务的最大数量
	uint client_need_server_num;//当前进程为client,发起的服务数量
	struct client_server server_need_start[CLIENT_SERVER_MAX];	//记录客户端需要服务的其实位置
};
*/

struct bind_percpu *cacu_percpu_address(const struct content_sendto_user *content, const void *map_address)
{
    struct bind_percpu *percpu;
    uint offset = content->percpu_start_offset + content->per_cpu_size * content->cpu_id;
    
    return (struct bind_percpu *)(map_address + offset);
}

struct rpc_node *cacu_node_address(const struct content_sendto_user *content, const void *map_address, uint node_id)
{
    struct rpc_node *node;
    uint offset = content->node_start_offset + content->node_size * node_id;
    if (node_id == BIND_INVLIED_ID) return NULL;
    return (struct rpc_node *)(map_address + offset);
}

char *cacu_data_address(const struct content_sendto_user *content, const void *map_address, uint data_id)
{
    uint offset = content->data_start_offset + BINDER_BLOCK_SIZE * data_id;
    if (offset >= content->map_size) return NULL;

    return (char *)(map_address + offset);
}

struct rpc_node *cacu_next_client_node(const struct content_sendto_user *content, const void *map_address, const struct rpc_node *node)
{
    return cacu_node_address(content, map_address, node->next_client_req);
}

void make_rpc_para(struct rpc_para *para, void **output, uint cnt)
{
    if (cnt < 5) {
        printf("cnt is too small %d\n", cnt);
    }
    para[0].rpc_id = 10013;
    para[0].input = "hello_world-10013";
    para[0].input_len = 100;
    para[0].output = output[0];
    para[0].output_len = 100;

    para[1].rpc_id = 10016;
    para[1].input = "hello_world-10016";
    para[1].input_len = 200;
    para[1].output = output[1];;
    para[1].output_len = 200;

    para[2].rpc_id = 10023;
    para[2].input = "hello_world-10023";
    para[2].input_len = 300;
    para[2].output = output[2];;
    para[2].output_len = 300;

    para[3].rpc_id = 10123;
    para[3].input = "hello_world-10123";
    para[3].input_len = 300;
    para[3].output = output[3];;
    para[3].output_len = 300;

    para[4].rpc_id = 10133;
    para[4].input = "hello_world-10133";
    para[4].input_len = 300;
    para[4].output = output[4];;
    para[4].output_len = 300;
}


uint make_percpu(int fd, const void *map_address, struct rpc_para *para, uint cnt)
{
    uint ret;
    uint i;
    struct bind_percpu percpu;
    struct bind_percpu *bind_percpu;
    struct content_sendto_user content;

    memset(&percpu, 0 , sizeof(percpu));

    ret = read(fd, &content, sizeof(content));
    if (ret == (uint)-1) {
        printf("read address is %d;\n", content.map_size);
        return (uint)-1;
    }

    bind_percpu = cacu_percpu_address(&content, map_address);

    percpu.cpuid = content.cpu_id;
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

uint set_input_para(const struct content_sendto_user *content, const void *map_address, struct rpc_para *para, uint cnt, uint start_node_id)
{
    uint i = 0;
    struct rpc_node *node = NULL;
    char *input;
    char *output;
    uint next_node_id = start_node_id;


    while (i < cnt) {
        node = cacu_node_address(content, map_address, next_node_id);
        if (node == NULL) break;
        input = cacu_data_address(content, map_address, node->input_data);
        strcpy(input, para[i].input);
        printf("node id is %d, input str is %s \n", node->node_id, input);
        output = cacu_data_address(content, map_address, node->output_data);
        para[i].output = output;
        next_node_id = node->next_client_req;
        i++;
    } 

    if (i != cnt) return BIND_RET_ERROR;

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
        printf("input rpc_id:%d, output str:%s \n", para[i].rpc_id, (char *)para[i].output);
    }
}

uint reg_req(int fd, const void *map_address, const struct content_sendto_user *content)
{
    struct rpc_para para[5];
    char *output[5];
    uint node_id;
    uint ret;
    uint i;
    for(i = 0; i < 5; i++) {
        outpu[i] = (char *)malloc(300);
    }

    make_rpc_para(para, output, sizeof(para) / sizeof(para[0]));

    do {
        ret = make_percpu(fd, map_address, para, sizeof(para) / sizeof(para[0]));

        if (ret != BIND_RET_SUCCESS) {
            usleep(200);
            continue;
        }

        ret = ioctl(fd, RPC_REQUEST_CMD, &node_id);
        printf("printf node id  %d\n",node_id);
    } while (ret != BIND_RET_SUCCESS);

    ret = set_input_para(content, map_address, para, sizeof(para) / sizeof(para[0]), node_id);
    if (ret != BIND_RET_SUCCESS) {
        printf("data error\n");
        return BIND_RET_ERROR;
    }

    print_input(para, sizeof(para) / sizeof(para[0]));

    ret = ioctl(fd, RPC_CLIENT_WAIT, NULL);
    if (ret != BIND_RET_SUCCESS) {
        return BIND_RET_ERROR;
    }

    print_output(para, sizeof(para) / sizeof(para[0]));

    ret = ioctl(fd, RPC_CLIENT_COMPLETE, NULL);
    if (ret != BIND_RET_SUCCESS) {
        return BIND_RET_ERROR;
    }

    return BIND_RET_SUCCESS;
}


int main(void) {
    int fd;
    uint node_id;
    void *map_address;
    int ret;
    struct content_sendto_user content;    
    struct bind_percpu *bind_percpu;

    struct rpc_node *rpc_node;

    fd = open(BINDER_DRIVER,O_RDWR);
    if (fd < 0) {
        printf("open file error \n");
        return -1;
    }

    ret = read(fd, &content, sizeof(content));
    if (ret == -1) {
        printf("read address is %d;\n", content.map_size);
        return -1;
    }

    map_address = mmap(NULL, content.map_size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_LOCKED, fd, 0);
    if (map_address == MAP_FAILED) {
        printf("map error %d;\n", content.map_size);
        return -1;
    }

    reg_req(fd, map_address, &content);

    munmap(map_address, content.map_size);

    close(fd);

    return 0;
}