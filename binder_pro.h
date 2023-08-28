#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/miscdevice.h>
#include <linux/pci.h>
#include <linux/wait.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/poll.h>
#include <linux/threads.h>
#include <linux/types.h>
#include <linux/list.h>

#include <asm/io.h>
#include <asm/uaccess.h>

#ifndef BINDER_PRO_H
#define BINDER_PRO_H

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

struct binder_buffer_info{
	void *buffer_address;
	uint buffer_size;
	uint block_num;
	uint pfn;
};

struct bind_super{
	struct binder_buffer_info *binder_buffer_info;
	spinlock_t lock_node_map;		/* 自旋锁 */
	spinlock_t lock_data_map;		/* 自旋锁 */

	uint cpu_num;			//记录cpu数量
	void* per_cpu_start; 		//每cpu在共享内存中的起始位置
	uint per_cpu_size;			//recard one cpu area size
	uint per_cpu_size_total;	//
	int per_cpu_id;

	void* node_map_start;	//记录业务节点位图起点
	uint node_map_size;		//
	int node_map_id;
	void* node_start;		//记录业务节点起点
	int node_id;
	uint node_num_total;	//记录业务节点总数量
	uint node_num_left;		//记录业务节点剩余数量
	uint per_node_size;		//recard one node size
	uint node_size_total;	//

	void* data_map_start;	//记录数据位图起点
	int data_map_id;
	uint data_map_size;		//
	void* data_start;			//记录数据块起点
	int data_id;
	uint data_num_total;	//记录数据块总数据量
	uint data_num_left;		//记录数据块剩余数量
	uint data_size_total;	//
};

struct client_req_hash {
	struct list_head list_head;
	pid_t pid;
	uint req_cnt;
	atomic_t req_done_num;
	uint start_req_node;
};



struct binder_pro_dev{
	struct binder_buffer_info *binder_buffer_info;
	wait_queue_head_t r_wait;	/* 读等待队列头 */
};

struct server_item{
	struct list_head hash_list;
	struct list_head reg_list;
	spinlock_t item_lock;
	uint start_node_id;
	uint process_node_id;
	pid_t pid;
};

struct rpc_register {
	struct rb_node rb_node_rpc;
	struct list_head reg_list;
	uint rpc_id;
	pid_t pid;
};

struct binder_dev {
	struct binder_buffer_info binder_buffer_info;
	struct bind_super binder_super;
	struct bind_percpu binder_percpu;
	struct rb_root rpc_root;
	struct list_head  client_hash[BINDER_HASH_SIZE];
	struct list_head  server_hash[BINDER_HASH_SIZE];
	wait_queue_head_t r_client_rq;	/* 读等待队列头 */
	wait_queue_head_t r_server_rq;
};

uint bitmap_scan_set(unsigned char *bitmap, uint bit_size, uint32_t cnt, spinlock_t *lock , uint *bit_left);
void bitmap_set_zero_lock(unsigned char *bitmap, uint bit_idx, spinlock_t *lock, uint *bit_left);
uint node_offset_start(struct bind_super *super);
uint data_offset_start(struct bind_super *super);
uint caculate_offset_by_node_id(struct bind_super *super, uint idx);
uint caculate_offset_by_data_id(struct bind_super *super,uint idx);
void *caculate_node_address(struct bind_super *super, uint idx);
void *caculate_data_address(struct bind_super *super, uint idx);
void bitmap_test(void);



struct rpc_node *malloc_node(struct bind_super *super);
struct rpc_node *bind_rpc_source_malloc(struct bind_super *super,struct client_server *client_req);
void bind_nodes_free(struct bind_super *super, struct rpc_node **rpc_node, uint cnt);
void bind_malloc_test(struct bind_super *super);
void bind_nodemap_free(struct bind_super *super, uint map_idx);
void bind_datamap_free_size(struct bind_super *super, uint map_idx, uint size);
void bind_node_free_id_chain(struct bind_super *super, uint node_id);

bool register_rpc(struct rb_root *root, struct rpc_register *rpc);
void delete_rpc_by_id(struct rb_root *rb_root, uint rpc_id);
void rpc_test_multy(struct rb_root *rb_root);

void init_super_block(struct bind_super *super, struct binder_buffer_info *buffer_info);
void init_binder_block(struct binder_buffer_info *buffer);
void init_test(struct bind_super *super);

void delete_rpc_by_id(struct rb_root *rb_root, uint rpc_id);
struct rpc_register *rpc_search(struct rb_root *root, uint rpc_id);
bool register_rpc(struct rb_root *root, struct rpc_register *rpc);
struct rpc_register *get_rpc_reg(struct server_item *server_item, uint rpc_id);
struct server_item *find_server_item(struct list_head  *server_hash, pid_t sever_pid);
uint insert_rpc_hlist(struct list_head *server_heads, struct rpc_register *rpc_reg, pid_t pid);
void check_server_data_all(struct bind_super *super, struct list_head *server_hash);
uint req_to_server(struct list_head *server_head, struct rpc_node *rpc_node);
void start_server_rpc(struct server_item *server_item);

uint client_req_server(struct binder_dev *binder_devp, struct client_req_hash **cli_req);
uint client_test_set_data(struct bind_super *super, uint *rpc_id, uint rpc_cnt, pid_t client_pid);
void rpc_node_check(struct rpc_node **rpc_node, uint rpc_cnt);
void check_client_data_all(struct bind_super *super, struct list_head *client_hash);
struct client_req_hash *find_req_data(struct list_head  *client_hash, pid_t pid);
uint client_mark(struct binder_dev *binder_devp, uint node_id);

uint server_reg_test(struct binder_dev *binder_devp);
#endif