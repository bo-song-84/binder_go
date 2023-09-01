#ifndef BINDER_APP_H
#define BINDER_APP_H

#define BINDER_DRIVER "/dev/binder_pro"

struct rpc_para
{
    uint rpc_id;
    const void *input;
    uint input_len;
    void *output;
    uint output_len;
};

struct binder_rpc_register {
    uint rpc_id;
    void*  (*rpc_call_fun) (uint rpc_id, const void *input, uint input_len);
};

#define BINDER_BLOCK_SIZE 		128
#define BINDER_NAME 			"binder_pro"
#define CLIENT_SERVER_MAX       5
#define BITMAP_MASK 			1
#define BINDER_HASH_SIZE		100

#define RPC_STATUS_CLIENT_REQ	1
#define RPC_STATUS_SERVER_CODE	2
#define RPC_STATUS_SERVER_OVER	3
#define RPC_STATUS_CLIENT_OVER	4
#define RPC_STATUS_ERROR		5

#define RPC_ROLE_CLIENT			0
#define RPC_ROLE_SERVER			1

#define RB_TREE_INSERT_CMD 		(_IO(0XEF, 0x1))	/* insert */
#define RB_TREE_EREASE_CMD		(_IO(0XEF, 0x2))	/* erease */
#define RB_TREE_SEARCH_CMD	    (_IO(0XEF, 0x3))	/* serch */
#define RPC_REQUEST_CMD	        (_IO(0XEF, 0x4))	/* server req */
#define RPC_CLIENT_DSP	        (_IO(0XEF, 0x5))	/* server display from client */
#define RPC_SERVER_DSP	        (_IO(0XEF, 0x6))	/* server display from server */
#define RPC_SERVER_WAIT			(_IO(0XEF, 0x7))
#define RPC_SERVER_COMPLETE		(_IO(0XEF, 0x8))
#define RPC_CLIENT_WAIT			(_IO(0XEF, 0x9))
#define RPC_CLIENT_COMPLETE		(_IO(0XEF, 0x10))

#define BIND_RET_SUCCESS		0
#define BIND_MAP_SUCCESS		0
#define BIND_RET_ERROR			(uint)(-1)
#define BIND_LONG_RET_ERROR		-1
#define BIND_MAP_ERROR			(uint)(-1)

#define BIND_INVLIED_ID			(uint)(-1)

#define _ALIGN_UP(addr,size)	(((addr)+((size)-1))&(~((size)-1)))
/* align addr on a size boundary - adjust address up if needed */
#define _ALIGN(addr,size)     	_ALIGN_UP(addr,size)
#define ALIGN_PER32(addr)		_ALIGN(addr, 32)
#define ALIGN_BLOCKSIZE(addr)	_ALIGN(addr, BINDER_BLOCK_SIZE)

struct content_sendto_user {
	uint map_size;
	uint cpu_id;
	uint node_size;
	uint node_start_offset;
	uint data_start_offset;
	uint per_cpu_size;
	uint percpu_start_offset;
};

/*业务节点主要记录当前需要处理的rpc业务并且记录业务处理的状态和业务处理需要的数据 */
struct rpc_node {
	char rpc_status;	//记录当前节点处理状态，客户端请求中、服务端处理中、服务端处理完、客户端已确认
	uint node_id;
	uint rpc_id;
	pid_t client_pid;	//客户端pid
	pid_t service_pid;	//服务端pid
	uint input_data;	//客户端输入数据
	uint input_data_size;//输入长度
	uint output_data;	//服务端返回值和数据
	uint output_data_size; //输出长度
	uint next_client_req;	//客户端的下一个请求
	uint next_service_req;	//服务端下一个需要处理的请求
};

struct client_server {
	uint rpc_id;		//需要调用的rpc id
	uint input_size;	//输入参数的长度
	uint output_size;	//输出参数的长度
};

struct bind_percpu{		/*  */
	//mutex_lock lock;  //主要是为了防止进程被抢占导致的数据异常，如果开启禁止抢占内核，这个锁就不需要了
	uint cpuid; 		//cpuid
	pid_t pid;			//当前使用进程的pid
	uint role;			//当前进程的角色  server、client or no rpc
	uint server_start_id;// 当前进程为server时，第一个需要服务的node节点id
	uint client_server_max; //一个客户端可以同时申请服务的最大数量
	uint client_need_server_num;//当前进程为client,发起的服务数量
	struct client_server server_need_start[CLIENT_SERVER_MAX];	//记录客户端需要服务的其实位置
};

uint call_binder_go(struct rpc_para *rpc_para, uint cnt);
int read_content(int fd,  struct content_sendto_user *content);
struct bind_percpu *cacu_percpu_address(const struct content_sendto_user *content, const void *map_address);
struct rpc_node *cacu_node_address(const struct content_sendto_user *content, const void *map_address, uint node_id);
void *cacu_data_address(const struct content_sendto_user *content, const void *map_address, uint data_id);
uint start_binder_server(struct binder_rpc_register *rpc_calls, uint cnt);


#endif