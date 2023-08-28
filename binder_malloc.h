#include "binder_pro.h"

#ifndef BINDER_MALLOC_H
#define BINDER_MALLOC_H

uint malloc_data(struct bind_super *super, uint cnt)
{
    uint bit_idx;
    if (cnt == 0) return 0;
	bit_idx = bitmap_scan_set((unsigned char *)super->data_map_start,super->data_num_total,cnt, &super->lock_data_map, &super->data_num_left);
    printk("bit idx %d, cnt num is %d\n",bit_idx, cnt);
    if (bit_idx == BIND_MAP_ERROR) {
        printk("not enough space \n");
        return BIND_MAP_ERROR;
    }
    return bit_idx;
}

uint malloc_data_bysize(struct bind_super *super, uint size) {
	uint size_align_block;
	uint bit_idx;

	size_align_block = ALIGN_BLOCKSIZE(size);
    if (size_align_block == 0) {
        return 0;
    }

	bit_idx = malloc_data(super, size_align_block / BINDER_BLOCK_SIZE);
    if (bit_idx == BIND_MAP_ERROR) {
        printk("not enough space \n");
        return BIND_MAP_ERROR;
    }
    return bit_idx;
}

struct rpc_node *bind_rpc_source_malloc(struct bind_super *super,struct client_server *client_req) {
	struct rpc_node *node;
	uint offset;
    uint bit_idx;

	bit_idx = bitmap_scan_set((unsigned char *)super->node_map_start,super->node_num_total,1, &super->lock_node_map, &super->node_num_left);
    printk("node idx = %d\n",bit_idx);
	if (bit_idx == BIND_MAP_ERROR) {
		printk("binder node out of buffer\n");
		return NULL;
	}
	//offset = super->per_cpu_size_total + super->node_map_size + super->data_map_size + bit_idx * super->per_node_size;
    offset = super->node_id * BINDER_BLOCK_SIZE + bit_idx * super->per_node_size;
    printk("node offset = %d\n",offset);
	node = (struct rpc_node *)(super->binder_buffer_info->buffer_address + offset);
	node->node_id = bit_idx;
    printk("node idx = %d\n",node->node_id);

    bit_idx = malloc_data_bysize(super, client_req->input_size);
    if (bit_idx == BIND_MAP_ERROR) {
        printk("not enough space \n");
        bind_nodemap_free(super, node->node_id);
        return NULL;
    }
    node->input_data = bit_idx;
    node->input_data_size = client_req->input_size;
    printk("input idx = %d, node input idx = %d\n",bit_idx, node->input_data);


    bit_idx = malloc_data_bysize(super, client_req->output_size);
    if (bit_idx == BIND_MAP_ERROR) {
        printk("not enough space \n");
        bind_nodemap_free(super, node->node_id);
        bind_datamap_free_size(super, node->input_data, node->input_data_size);
        return NULL;
    }
    node->output_data = bit_idx;
    printk("output idx = %d, node output idx = %d\n",bit_idx, node->output_data);
    node->output_data_size = client_req->output_size;

	return node;
}


void bind_datamap_free(struct bind_super *super, uint map_idx, uint cnt)
{
    uint i;
    for (i = 0; i < cnt; i++) {
        bitmap_set_zero_lock((unsigned char *)super->data_map_start, map_idx + i, &super->lock_data_map, &super->data_num_left);
    }
}

void bind_datamap_free_size(struct bind_super *super, uint map_idx, uint size)
{
    uint cnt;
    cnt = ALIGN_BLOCKSIZE(size);
    cnt /= BINDER_BLOCK_SIZE;
    bind_datamap_free(super, map_idx, cnt);
}

void bind_nodemap_free(struct bind_super *super, uint map_idx)
{
    bitmap_set_zero_lock((unsigned char *)super->node_map_start, map_idx, &super->lock_node_map, &super->node_num_left);
}

void bind_node_free(struct bind_super *super, struct rpc_node *rpc_node)
{
    uint input_size;
    uint output_size;

    input_size = rpc_node->input_data_size;
    output_size = rpc_node->output_data_size;
    
    if (input_size != 0) {
        bind_datamap_free_size(super, rpc_node->input_data, input_size );
    }

    if (output_size != 0) {
        bind_datamap_free_size(super, rpc_node->output_data, output_size);
    }

    bind_nodemap_free(super, rpc_node->node_id);

}

void bind_node_chain_free(struct bind_super *super, struct rpc_node *rpc_node)
{
    struct rpc_node *node = rpc_node;
    uint next_node_id;
    while (node != NULL) {
        next_node_id = node->next_client_req;
        bind_node_free(super, node);
        node = (struct rpc_node *)caculate_node_address(super, next_node_id);
    }
}

void bind_node_free_id_chain(struct bind_super *super, uint node_id)
{
    struct rpc_node *rpc_node;

    rpc_node = (struct rpc_node *)caculate_node_address(super, node_id);
    bind_node_chain_free(super, rpc_node);
}

void bind_nodes_free(struct bind_super *super, struct rpc_node **rpc_node, uint cnt) {
    uint i;
    for(i = 0; i < cnt; cnt++) {
        bind_node_free(super, rpc_node[i]);
    }
}



/* 
struct client_server {
	uint rpc_id;		//需要调用的rpc id
	uint input_size;	//输入参数的长度
	uint output_size;	//输出参数的长度
};
 */
void bind_malloc_test(struct bind_super *super)
{
    struct rpc_node *node;
    struct client_server client_req;
    char *node_map = (char *)super->node_map_start;
    char *date_map = (char *)super->data_map_start;

    char node_value;
    char data_ivalue;
    char data_ovalue;


    client_req.input_size = 135;
    client_req.output_size = 55;
    client_req.rpc_id = 120;
    printk("malloc ======================================================\n");
    printk("node_map:%x,date_map:%x\n",(uint)node_map[0],(uint)date_map[0]);
    node = bind_rpc_source_malloc(super, &client_req);

    node_value = node_map[node->node_id / 8];
    data_ivalue = date_map[node->input_data / 8];
    data_ovalue = date_map[node->output_data / 8];


    printk("malloc ======================================================\n");
    printk("input_size:%d,output size:%d\n",client_req.input_size,client_req.output_size);
    printk("node id is %d, odd:%d, node value %x\n", node->node_id,node->node_id%8,(uint)node_value);
    printk("input id is %d, odd:%d, input value %x\n", node->input_data,node->input_data%8,(uint)data_ivalue);
    printk("output id is %d, odd:%d, output value %x\n", node->output_data,node->output_data%8,(uint)data_ovalue);
    printk("data block_left %d, node block left %d \n", super->data_num_left ,super->node_num_left);

    bind_node_free(super, node);
    printk("free ======================================================\n");
    printk("input_size:%d,output size:%d\n",client_req.input_size,client_req.output_size);
    printk("node id is %d, odd:%d, node value %x\n", node->node_id,node->node_id%8,(uint)node_value);
    printk("input id is %d, odd:%d, input value %x\n", node->input_data,node->input_data%8,(uint)data_ivalue);
    printk("output id is %d, odd:%d, output value %x\n", node->output_data,node->output_data%8,(uint)data_ovalue);
    printk("data block_left %d, node block left %d \n", super->data_num_left ,super->node_num_left);

}

#endif