#include "binder_pro.h"

#ifndef BINDER_INIT_H
#define BINDER_INIT_H


/*
buffer start is percpu and then node_map, data_map, node_area, data_area
-------------------------------------------------------------------------------------------------
*/

void init_binder_block(struct binder_buffer_info *buffer)
{
	struct page *pages;
	// char *address_test;
	pages = alloc_pages(GFP_KERNEL,3);
	buffer->buffer_address = page_address(pages);
	buffer->pfn = page_to_pfn(pages);
	buffer->buffer_size = PAGE_SIZE * 8;
	buffer->block_num = buffer->buffer_size / BINDER_BLOCK_SIZE;
	memset(buffer->buffer_address,0,buffer->buffer_size);
	/*  buffer test
	address_test = buffer->buffer_address + 3000;
	strcpy(address_test, "helloasdfjkj;kljdfakj;sdlfjka;sldkjf");
	*/
}

void init_super_block(struct bind_super *super, struct binder_buffer_info *buffer_info)
{
	uint per_cpu_size_total;
	uint per_cpu_size;
	uint rpc_node_size;
	uint rpc_num_perblock;
	uint node_block_num;
	uint node_map_size;
	uint data_map_size;
	uint node_size;
	uint block_left;


	spin_lock_init(&super->lock_node_map);
	spin_lock_init(&super->lock_data_map);
    super->binder_buffer_info = buffer_info;

	printk("=============================================================================================================\n");
	block_left = buffer_info->block_num;
	printk("block num :%d \block num:%d \n", block_left ,block_left);

	/* alloc per cpu area */
	//binder_super.cpu_num = num_possible_cpus();
	super->cpu_num = num_present_cpus();
    printk("cpu num is %d \n", super->cpu_num);
	super->per_cpu_start = buffer_info->buffer_address;
	strcpy((char *)super->per_cpu_start,"percpu_start");
	super->per_cpu_id = 0;
	per_cpu_size = sizeof(struct bind_percpu);
	per_cpu_size = ALIGN_PER32(per_cpu_size);
    printk("per cpu size is  %d \n", per_cpu_size);
	super->per_cpu_size = per_cpu_size;
	per_cpu_size_total = per_cpu_size * super->cpu_num;
	per_cpu_size_total = ALIGN_BLOCKSIZE(per_cpu_size_total);
	super->per_cpu_size_total = per_cpu_size_total;
	block_left -= (per_cpu_size_total / BINDER_BLOCK_SIZE);
	printk("block left :%d per_cpu_size_total:%d \n", block_left ,per_cpu_size_total);

	super->node_map_start = buffer_info->buffer_address + per_cpu_size_total;
	strcpy((char *)super->node_map_start,"dode_map_start");
	super->node_map_id = super->per_cpu_id + per_cpu_size_total/BINDER_BLOCK_SIZE;

	rpc_node_size = sizeof(struct rpc_node);
	rpc_node_size = ALIGN_PER32(rpc_node_size);
	super->per_node_size = rpc_node_size;
	rpc_num_perblock = BINDER_BLOCK_SIZE / rpc_node_size;

	/* per node prepare 2 blocks */
	node_block_num = block_left / (rpc_num_perblock * 2 + 1);		/* here caculate the rpc nodes consume the number of block, not the node num */
	node_size = node_block_num * rpc_node_size * rpc_num_perblock;
	super->node_num_total = node_block_num * rpc_num_perblock;
	block_left -= node_block_num;
	super->node_size_total =  node_size;
	printk("block left :%d node_size:%d \n", block_left ,node_size);
	

	node_map_size = ALIGN_BLOCKSIZE((super->node_num_total + 7) / 8);
	super->node_map_size = node_map_size;
	super->data_map_id = super->node_map_id + node_map_size/BINDER_BLOCK_SIZE;
	super->data_map_start = buffer_info->buffer_address + super->data_map_id * BINDER_BLOCK_SIZE;
	strcpy((char *)super->data_map_start,"data_map_start");
	
	block_left -= (node_map_size/BINDER_BLOCK_SIZE);
	printk("block left :%d node_map_size:%d \n", block_left ,node_map_size);

	data_map_size = ALIGN_BLOCKSIZE((block_left + 7) / 8);
	super->data_map_size = data_map_size;
	block_left -= (data_map_size/BINDER_BLOCK_SIZE);
	printk("block left :%d data_map_size:%d \n", block_left ,data_map_size);
	super->data_num_total =  block_left;

	super->node_id = super->data_map_id + data_map_size/BINDER_BLOCK_SIZE;
	super->node_start = buffer_info->buffer_address + super->node_id * BINDER_BLOCK_SIZE;
	strcpy((char *)super->node_start,"node_start");

	super->data_id = super->node_id + node_size / BINDER_BLOCK_SIZE;
	super->data_start =  buffer_info->buffer_address + super->data_id * BINDER_BLOCK_SIZE;
	strcpy((char *)super->data_start,"data_start");
	super->data_size_total = super->data_num_total * BINDER_BLOCK_SIZE;


	super->data_num_left = super->data_num_total;
	super->node_num_left = super->node_num_total;	

	printk("binder size\n percpu:%d \tnode map:%d \tdata map:%d \tnode:%d \n", 
			per_cpu_size_total, node_map_size, data_map_size,node_size);

	printk("binder addr\n percpu:%p \tnode map:%p \tdata map:%p \tnode:%p \t data:%p \n node num %d, \t data num %d \n", 
			super->per_cpu_start, super->node_map_start, super->data_map_start,
			super->node_start, super->data_start, super->node_num_total,super->data_num_total);
	return;
}

void init_test(struct bind_super *super)
{
    printk("percpu:%s \n", (char *)super->per_cpu_start);
    printk("node map :%s \n", (char *)super->node_map_start);
    printk("data map:%s \n", (char *)super->data_map_start);
    printk("node area:%s \n", (char *)super->node_start);
    printk("data area:%s \n", (char *)super->data_start);
}

#endif