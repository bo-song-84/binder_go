#include "binder_pro.h"

#ifndef BINDER_RB_TREE_H
#define BINDER_RB_TREE_H


////  rb_tree start   ////////////////////////////////////////////////////////////////////////////

bool register_rpc(struct rb_root *root, struct rpc_register *rpc)
{
  	struct rb_node **new = &(root->rb_node), *parent = NULL;

  	/* Figure out where to put new node */
  	while (*new) {
  		struct rpc_register *this = container_of(*new, struct rpc_register, rb_node_rpc);
  		uint rpc_id = this->rpc_id;

		parent = *new;
  		if (rpc->rpc_id < rpc_id)
  			new = &((*new)->rb_left);
  		else if (rpc->rpc_id > rpc_id)
  			new = &((*new)->rb_right);
  		else {
			printk("insert error rpc id %d, pid %d\n", rpc_id, rpc->pid);
  			return false;
		}
  	}

  	/* Add new node and rebalance tree. */
  	rb_link_node(&rpc->rb_node_rpc, parent, new);
  	rb_insert_color(&rpc->rb_node_rpc, root);

	return true;
}

struct rpc_register *rpc_search(struct rb_root *root, uint rpc_id)
{
  	struct rb_node *node = root->rb_node;

  	while (node) {
  		struct rpc_register *rpc_find = container_of(node, struct rpc_register, rb_node_rpc);


		if (rpc_id < rpc_find->rpc_id)
  			node = node->rb_left;
		else if (rpc_id > rpc_find->rpc_id)
  			node = node->rb_right;
		else
  			return rpc_find;
	}
	return NULL;
}

void delete_rpc(struct rb_root *rb_root, struct rpc_register *rpc) {
  	if (rpc) {
  		rb_erase(&rpc->rb_node_rpc, rb_root);
  		kfree(rpc);
  	}
}

void delete_rpc_by_id(struct rb_root *rb_root, uint rpc_id)
{
	struct rpc_register *rpc;
	rpc = rpc_search(rb_root, rpc_id);
	delete_rpc(rb_root, rpc);
}

void delete_rpc_by_pid( void /*struct rb_root *rb_root, uint rpc_id */)
{
	return;
}



void rpc_test(struct rb_root *rb_root, uint rpc_id)
{
    bool bool_ret;
    struct rpc_register *rpc_reg;

    rpc_reg = kmalloc(sizeof(struct rpc_register), GFP_KERNEL);
    if (!rpc_reg) {
        printk("malloc error\n");
    }
    memset(rpc_reg, 0, sizeof(struct rpc_register));
    rpc_reg->rpc_id = rpc_id;
    rpc_reg->pid = 500;
    bool_ret = register_rpc(rb_root,rpc_reg);
    if (!bool_ret) {
        kfree(rpc_reg);
        printk("register error, rpc id %d\n", rpc_id);
    }
}

void rpc_test_multy(struct rb_root *rb_root)
{
    uint id = 1000;
    struct rpc_register *rpc_reg;

    for (id = 0; id < 2000; id++) {
        rpc_test(rb_root, id);
    }
    rpc_test(rb_root, 1200);
    rpc_test(rb_root, 1300);
    rpc_test(rb_root, 1400);

    id = 1200;
    rpc_reg = rpc_search(rb_root, id);
    if (rpc_reg) {
        printk("find rpc id %d \n", rpc_reg->rpc_id);
    } else {
        printk("find rpc err\n");
    }

    delete_rpc_by_id(rb_root, id);
    rpc_reg = rpc_search(rb_root, id);
    if (rpc_reg) {
        printk("find second rpc id %d \n", rpc_reg->rpc_id);
    } else {
        printk("find second rpc err\n");
    }

}

////  rb_tree  end   ////////////////////////////////////////////////////////////////////////////////


#endif