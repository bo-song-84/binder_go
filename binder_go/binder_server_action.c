#include "binder_pro.h"
#ifndef BINDER_SERVER_ACTION_H
#define BINDER_SERVER_ACTION_H


#define RPC_LIST_ADD        1
#define NODE_LIST_INSERT    2
#define RPC_LIST_DEL        3
#define RPC_SERVER_GET      4

void server_item_lock(struct server_item *server_item,struct rpc_register *rpc_reg, struct rpc_node *rpc_node, uint action) {
    unsigned long flags;

    spin_lock_irqsave(&server_item->item_lock, flags);	/* 上锁 */    
    switch (action) {
        case RPC_LIST_ADD:
            list_add(&rpc_reg->reg_list, &server_item->reg_list);
            break;
        case NODE_LIST_INSERT:
            if (server_item->start_node_id == BIND_INVLIED_ID) {
                server_item->start_node_id = rpc_node->node_id;
            } else {
                rpc_node->next_service_req = server_item->start_node_id;
                server_item->start_node_id = rpc_node->node_id;
            }
            break;
        case RPC_LIST_DEL:
            list_del(&rpc_reg->reg_list); 
            break;
        case RPC_SERVER_GET:
            server_item->process_node_id = server_item->start_node_id;
            server_item->start_node_id = BIND_INVLIED_ID;
            break;
        default:
            printk("server action can not find:%d \n", action);
            break;
    }
    spin_unlock_irqrestore(&server_item->item_lock, flags);
}

void start_server_rpc(struct server_item *server_item)
{
    server_item_lock(server_item, NULL, NULL, RPC_SERVER_GET);
}

uint insert_rpc_hlist(struct list_head *server_heads, struct rpc_register *rpc_reg, pid_t pid)
{
	struct server_item *server_item;
	bool server_item_exist;

	struct list_head *server_head= &server_heads[pid % BINDER_HASH_SIZE];
	server_item_exist = false;
	list_for_each_entry(server_item, server_head, hash_list) {
		if (server_item->pid == pid) {
			server_item_exist = true;
			break;
		}
	}

	if (!server_item_exist) {
		server_item = kmalloc(sizeof(struct server_item), GFP_KERNEL);
		if (server_item == NULL) {
			printk("out of memory \n");
			return BIND_RET_ERROR;
		}
		server_item->pid = pid;
		spin_lock_init(&server_item->item_lock);
		INIT_LIST_HEAD(&server_item->reg_list);
		server_item->start_node_id = BIND_INVLIED_ID;
        //printk("server_item exist %d\n", server_item_exist);
		list_add(&server_item->hash_list, server_head);
	}

    server_item_lock(server_item,rpc_reg, NULL, RPC_LIST_ADD);

	return BIND_RET_SUCCESS;
}

/*=============================================================================*/
void insert_node_list(struct server_item *server_item, struct rpc_node *rpc_node)
{
    server_item_lock(server_item, NULL, rpc_node, NODE_LIST_INSERT);
}

void rpc_node_del(struct list_head *server_hash, struct rpc_register *rpc_reg, struct rb_root *rpc_root)
{
    struct server_item *server_item;

    delete_rpc_by_id(rpc_root, rpc_reg->rpc_id);
    server_item = find_server_item(server_hash, rpc_reg->rpc_id);
    if (server_item != NULL) {
        server_item_lock(server_item, rpc_reg, NULL, RPC_LIST_DEL);
    }
}

/*
struct server_item{
	struct list_head hash_list;
	struct list_head reg_list;
	spinlock_t item_lock;
	uint start_node_id;
	uint process_node_id;
	pid_t pid;
};
*/
uint req_to_server(struct list_head *server_head, struct rpc_node *rpc_node)
{
    pid_t pid;
    struct server_item *server_item;

    pid = rpc_node->service_pid;
    server_item = find_server_item(server_head, pid);
    if (server_item == NULL) {
        printk("can not find server item pid:%d \n", pid);
        return BIND_RET_ERROR;
    }
    insert_node_list(server_item, rpc_node);

    return BIND_RET_SUCCESS;
}

struct server_item *get_server_item(struct list_head  *server_head, pid_t sever_pid)
{
    struct server_item *server_item;

    list_for_each_entry(server_item, server_head, hash_list)  {
        if (server_item->pid == sever_pid) {
            return server_item;
        }
    } 

    return NULL;
}

struct rpc_register *get_rpc_reg(struct server_item *server_item, uint rpc_id)
{
    struct rpc_register *rpc_reg;
    list_for_each_entry(rpc_reg, &server_item->reg_list, reg_list) {
        if (rpc_reg->rpc_id == rpc_id) {
            return rpc_reg;
        }
    }

    return NULL;
}

struct server_item *find_server_item(struct list_head *server_hash, pid_t sever_pid)
{
    struct server_item *server_item;
    struct list_head *head;

    head = &server_hash[sever_pid % BINDER_HASH_SIZE];
    server_item = get_server_item(head, sever_pid);

    return server_item;
}

uint req_to_serverlist(struct list_head *server_hash, struct rpc_node **rpc_node, uint rpc_cnt, struct rb_root *rpc_root)
{
	uint rpc_id;
    uint i;
    pid_t server_pid;
    struct server_item *server_item;

	for (i = 0; i < rpc_cnt; i++) {
        server_item = find_server_item(server_hash, rpc_node[i]->service_pid);
        if (server_item == NULL) {
            rpc_node[i]->rpc_status = RPC_STATUS_ERROR;
            printk("not find the server item rpc_id %d, server pid %d\n", rpc_id, server_pid);
            continue;
        }

        insert_node_list(server_item, rpc_node[i]);
	}

    return BIND_RET_SUCCESS;
}

//struct server_item *find_server_item(struct list_head  *server_hash, pid_t sever_pid)
uint check_server_data(struct bind_super *super, struct list_head  *server_hash, pid_t pid)
{
    struct server_item *server_item;
    struct rpc_node *rpc_node;
    uint req_node_id;
    uint offset;

    server_item = find_server_item(server_hash, pid);
    if (server_item == NULL) {
        printk("find server item error pid:%d\n", pid);
        return BIND_RET_ERROR; 
    }

    req_node_id = server_item->start_node_id;
    while(req_node_id != BIND_INVLIED_ID) {
        offset = caculate_offset_by_node_id(super, req_node_id);
        if (offset >= 8 * PAGE_SIZE) {
            printk("offset caculate error:%d\n", offset);
            return BIND_RET_ERROR;
        }
        rpc_node = (struct rpc_node *)(super->binder_buffer_info->buffer_address + offset);
        rpc_node_check(&rpc_node, 1);
        req_node_id = rpc_node->next_service_req;
    }

    return BIND_RET_SUCCESS;
}


void check_server_data_all(struct bind_super *super, struct list_head *server_hash)
{
    struct server_item *server_item;
    struct list_head *server_head;
    pid_t i;
    for(i = 0; i < BINDER_HASH_SIZE; i++) {
        server_head = &server_hash[i];
        if (list_empty(server_head)) continue;

        list_for_each_entry(server_item, server_head, hash_list)  {
            if (server_item != NULL) {
                check_server_data(super, server_hash, server_item->pid);
            }
        } 
    }
}


#endif