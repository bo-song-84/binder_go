#include "binder_pro.h"

#ifndef BINDER_CLIENT_ACTION_H
#define BINDER_CLIENT_ACTION_H

struct client_req_hash *client_req_reg(struct list_head *client_hash, struct rpc_node *rpc_node, uint rpc_cnt)
{
	struct client_req_hash *client_req;
	struct list_head *list =  &client_hash[rpc_node->client_pid % BINDER_HASH_SIZE];

    client_req = find_req_data(client_hash, rpc_node->client_pid);
    if (client_req == NULL) {
        client_req = kmalloc(sizeof(struct client_req_hash), GFP_KERNEL);
        if (client_req == NULL) {
            printk("out of memory \n");
            return NULL;
        }
        list_add(&client_req->list_head, list);
    }

	client_req->pid = rpc_node->client_pid;
	client_req->req_cnt = rpc_cnt;
    atomic_set(&client_req->req_done_num, 0);
	client_req->start_req_node = rpc_node->node_id;

    return client_req;
}


/*
业务节点主要记录当前需要处理的rpc业务并且记录业务处理的状态和业务处理需要的数据 
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
*/
void rpc_node_check(struct rpc_node **rpc_node, uint rpc_cnt)
{
    uint i;
    uint node_id, rpc_id;
    uint input_id, input_len;
    uint output_id, output_len;
    pid_t client_pid, server_pid;

    for(i = 0; i < rpc_cnt; i++) {
        node_id = rpc_node[i]->node_id;
        rpc_id = rpc_node[i]->rpc_id;
        input_id = rpc_node[i]->input_data;
        input_len = rpc_node[i]->input_data_size;
        output_id = rpc_node[i]->output_data;
        output_len = rpc_node[i]->output_data_size;
        client_pid = rpc_node[i]->client_pid;
        server_pid = rpc_node[i]->service_pid;

        printk("node id:%d, rpc_id:%d, input id:%d, input len:%d, output id:%d, output size:%d, client pid:%d, server pid:%d \n",
                node_id, rpc_id, input_id, input_len, output_id, output_len, client_pid, server_pid);
    }
}

/*
struct binder_dev {
	struct binder_buffer_info binder_buffer_info;
	struct bind_super binder_super;
	struct bind_percpu binder_percpu;
	struct rb_root rpc_root;
	struct list_head  client_hash[BINDER_HASH_SIZE];
	struct list_head  server_hash[BINDER_HASH_SIZE];
	wait_queue_head_t r_client_rq;	读等待队列头 
	wait_queue_head_t r_server_rq;
};
*/

uint client_req_server(struct binder_dev *binder_devp, struct client_req_hash **cli_req)
{
	int cpu = smp_processor_id();
	uint server_num;
    uint offset;
    void *this_address;
    struct list_head *client_hash = binder_devp->client_hash;
    struct list_head *sever_hash = binder_devp->server_hash;
    struct bind_super *super = &binder_devp->binder_super;
    struct rb_root *rpc_root = &binder_devp->rpc_root;
    struct bind_percpu *percpu_data;
    struct client_server *client_req;
    struct rpc_register *rpc_reg;
	struct rpc_node *rpc_nodes[CLIENT_SERVER_MAX] = {0};
	uint ret,i;

	offset = super->per_cpu_size * (uint)cpu;
	this_address = super->per_cpu_start + offset;
	percpu_data = (struct bind_percpu *)this_address;
	server_num = percpu_data->client_need_server_num;
	client_req = percpu_data->server_need_start;

    if (percpu_data->pid != current->pid) {
        printk("per cpu data crashed, pid should be:%d,the real pid is %d\n",current->pid, percpu_data->pid);
        return BIND_RET_ERROR;
    }

    if (server_num > CLIENT_SERVER_MAX) {
        printk("can not handle so many services:%d > max num%d\n", server_num, CLIENT_SERVER_MAX);
        return BIND_RET_ERROR;
    }

    if (server_num == 0) {
        printk("can not handle zero servers\n");
        return BIND_RET_ERROR;
    }

	for (i = 0; i < server_num; i++) {
        client_req = &percpu_data->server_need_start[i];
		rpc_nodes[i] = bind_rpc_source_malloc(super, client_req);
		if (rpc_nodes[i] == NULL) {
			printk("out of bander buffer \n");
            if (i == 0) return BIND_RET_ERROR;
            bind_nodes_free(super, rpc_nodes, i - 1);
            return BIND_RET_ERROR;
		}
        
        rpc_nodes[i]->rpc_status = RPC_STATUS_CLIENT_REQ;
        rpc_nodes[i]->rpc_id = client_req->rpc_id;
        rpc_reg =  rpc_search(rpc_root, rpc_nodes[i]->rpc_id);
        if (rpc_reg == NULL) {
            printk("server error, can not find rpc id %d \n", rpc_nodes[i]->rpc_id);
            if (i == 0) return BIND_RET_ERROR;
            bind_nodes_free(super, rpc_nodes, i - 1);
            return BIND_RET_ERROR;
        }
        //rpc_nodes[i]->client_pid = current->pid;
        rpc_nodes[i]->client_pid = percpu_data->pid;
        rpc_nodes[i]->service_pid = rpc_reg->pid;
	}

    rpc_nodes[server_num - 1]->next_client_req = BIND_INVLIED_ID;
    rpc_nodes[server_num - 1]->next_service_req = BIND_INVLIED_ID;
    for(i = 0; i < server_num - 1; i++) {
        rpc_nodes[i]->next_client_req = rpc_nodes[i + 1]->node_id;
        rpc_nodes[i]->next_service_req = BIND_INVLIED_ID;
    }

    *cli_req = client_req_reg(client_hash, rpc_nodes[0], server_num);
    if (*cli_req == NULL) {
        bind_nodes_free(super, rpc_nodes, server_num);
        return BIND_RET_ERROR;
    }
    //uint req_to_server(struct list_head *server_head, struct rpc_node *rpc_node)

    for(i = 0; i < server_num; i++) {
        ret = req_to_server(sever_hash, rpc_nodes[i]);
        if (ret != BIND_RET_SUCCESS) {
            printk("can not find server pid:%d\n", rpc_nodes[i]->service_pid);
            bind_nodes_free(super, rpc_nodes, server_num);
            return BIND_RET_ERROR;
        }
    }

    rpc_node_check(rpc_nodes, server_num);
    return ret;
}

uint client_mark_server(struct binder_dev *binder_devp, struct rpc_node *node, bool *wakeup) /*mark the node has been completed*/
{
    struct client_req_hash *client_req;

    client_req = find_req_data(binder_devp->client_hash, node->client_pid);
    atomic_inc(&client_req->req_done_num);
    if (client_req->req_cnt == atomic_read(&client_req->req_done_num)) {
        *wakeup = true;
    }

    return BIND_RET_SUCCESS;
}

uint client_mark(struct binder_dev *binder_devp, uint node_id)
{
    struct rpc_node *node;
    uint node_id_mark = node_id;
    bool wakeup = false;
    
    while (node_id_mark != BIND_INVLIED_ID && node_id_mark < binder_devp->binder_super.node_num_total) {
        node = (struct rpc_node *)caculate_node_address(&binder_devp->binder_super, node_id_mark);
        client_mark_server(binder_devp, node, &wakeup);
        node_id_mark = node->next_service_req;
    }

    if (wakeup) wake_up_interruptible(&binder_devp->r_client_rq);

    return BIND_RET_SUCCESS;
}
/*
struct client_server {
	uint rpc_id;		//需要调用的rpc id
	uint input_size;	//输入参数的长度
	uint output_size;	//输出参数的长度
};

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
uint client_test_set_data(struct bind_super *super, uint *rpc_id, uint rpc_cnt, pid_t client_pid)
{
    int cpu = smp_processor_id();
    uint offset;
    void *this_address;
    struct bind_percpu *percpu_data;
    uint i;

    if (rpc_cnt > CLIENT_SERVER_MAX) {
        printk("can not handle so many client req ,req num:%d,max req num:%d",rpc_cnt, CLIENT_SERVER_MAX);
        return BIND_RET_ERROR;
    }

    offset = super->per_cpu_size * (uint)cpu;
	this_address = super->per_cpu_start + offset;
	percpu_data = (struct bind_percpu *)this_address;

    percpu_data->cpuid = cpu;
    percpu_data->pid = client_pid;
    percpu_data->role = RPC_ROLE_CLIENT;
    percpu_data->client_need_server_num = rpc_cnt;
    
    for (i = 0; i < rpc_cnt; i++) {
        percpu_data->server_need_start[i].rpc_id = rpc_id[i];
        percpu_data->server_need_start[i].input_size = 100 * (i + 1);
        percpu_data->server_need_start[i].output_size = 100 * (i + 1);
    }

    return 0;
}

/*
struct client_req_hash {
	struct list_head list_head;
	pid_t pid;
	uint req_cnt;
	atomic_t req_done_num;
	uint start_req_node;
};
*/
struct client_req_hash *find_req_data(struct list_head  *client_hash, pid_t pid)
{
    struct client_req_hash *client_req;
    struct list_head *client_head = &client_hash[pid % BINDER_HASH_SIZE];

    list_for_each_entry(client_req, client_head, list_head)  {
        if (client_req->pid == pid) {
            return client_req;
        }
    } 
    return NULL;
}

/*
struct client_req_hash {
	struct list_head list_head;
	pid_t pid;
	uint req_cnt;
	atomic_t req_done_num;
	uint start_req_node;
};
void *caculate_node_address(struct bind_super *super, uint idx);
uint caculate_offset_by_node_id(struct bind_super *super, uint idx);
*/
uint check_client_data(struct bind_super *super, struct list_head  *client_hash, pid_t pid)
{
    struct client_req_hash *client_req;
    struct rpc_node *rpc_node;
    uint req_node_id;
    uint offset;

    client_req = find_req_data(client_hash, pid);
    if (client_req == NULL) {
        printk("no req in this pid:%d\n", pid);
        return BIND_RET_ERROR; 
    }

    req_node_id = client_req->start_req_node;
    while(req_node_id != BIND_INVLIED_ID) {
        offset = caculate_offset_by_node_id(super, req_node_id);
        if (offset >= 8 * PAGE_SIZE) {
            printk("offset caculate error:%d\n", offset);
            return BIND_RET_ERROR;
        }
        rpc_node = (struct rpc_node *)(super->binder_buffer_info->buffer_address + offset);
        rpc_node_check(&rpc_node, 1);
        req_node_id = rpc_node->next_client_req;
    }

    return BIND_RET_SUCCESS;
}

void check_client_data_all(struct bind_super *super, struct list_head *client_hash)
{
    struct client_req_hash *client_req;
    struct list_head *client_head;
    pid_t i;
    for(i = 0; i < BINDER_HASH_SIZE; i++) {
        client_head = &client_hash[i];
        if (list_empty(client_head)) continue;

        list_for_each_entry(client_req, client_head, list_head)  {
            if (client_req != NULL) {
                check_client_data(super, client_hash, client_req->pid);
            }
        } 
    }
}


#endif

