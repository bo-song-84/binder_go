#include "binder_pro.h"
#ifndef BINDER_COMBIN_TEST_H
#define BINDER_COMBIN_TEST_H


static long binder_pro_reg_test(struct binder_dev *binder_devp, unsigned int cmd, uint rpc_id,pid_t pid) {
	struct rb_root *rpc_rb_root = &binder_devp->rpc_root;
	struct rpc_register *rpc_reg;
	struct client_req_hash *client_req;
	uint ret;
	bool bool_ret;

	switch (cmd) {
		case RB_TREE_INSERT_CMD: {
			rpc_reg = kmalloc(sizeof(struct rpc_register), GFP_KERNEL);
			if (!rpc_reg) {
				return -EINVAL;
			}
			memset(rpc_reg, 0, sizeof(struct rpc_register));
			rpc_reg->rpc_id = rpc_id;
			rpc_reg->pid = pid;
			bool_ret = register_rpc(rpc_rb_root, rpc_reg);
			if (!bool_ret) {
				kfree(rpc_reg);
				return -EINVAL; 
			}
			ret = insert_rpc_hlist(binder_devp->server_hash, rpc_reg, rpc_reg->pid);
			if (ret != BIND_RET_SUCCESS) {
				delete_rpc_by_id(rpc_rb_root,  rpc_id);
				kfree(rpc_reg);
				return -EINVAL;
			}

			break;
		}
		case RB_TREE_EREASE_CMD:
			delete_rpc_by_id(rpc_rb_root,  rpc_id);
			break;
		case RB_TREE_SEARCH_CMD:
			rpc_reg = rpc_search(rpc_rb_root, rpc_id);
			if (!rpc_reg) {
				printk("finder node error\n");
				return -EINVAL;
			}
			break;
		case RPC_REQUEST_CMD:
			ret = client_req_server(binder_devp, &client_req);
			if (ret != BIND_RET_SUCCESS) {
				printk("client req server error\n");
				return -EINVAL;
			}
			break;
		default:
			break;
	}

	return BIND_RET_SUCCESS;
}


uint server_reg_test(struct binder_dev *binder_devp)
{
	uint rpc_id;
	uint ret;
	pid_t pid;
	struct server_item *server_item;
	struct rpc_register *rpc_reg;
	struct client_req_hash *client_req;

	struct list_head *client_head = binder_devp->client_hash;
	struct list_head *server_head = binder_devp->server_hash;

	uint rpc_arr[] = {1003,1033,1005,1015};

	for (pid = 100; pid < 1000; pid++) {
		for(rpc_id = pid * 10; rpc_id < pid * 10 + 10; rpc_id++) {
			binder_pro_reg_test(binder_devp, RB_TREE_INSERT_CMD, rpc_id, pid);
		}
	}

	//printk("state1 complete \n");

	for (pid = 100; pid < 1005; pid++) {
		server_item = find_server_item(server_head, pid);
		//printk("state2 complete \n");
		if (server_item == NULL) {
			printk("can not find server item pid %d\n", pid);
			continue;
		}
		for(rpc_id = pid * 10; rpc_id < pid * 10 + 15; rpc_id++) {
			rpc_reg = get_rpc_reg(server_item, rpc_id);
			//printk("state3 complete \n");
			if (rpc_reg == NULL) {
				//printk("can not find rpc reg %d,rpc id %d\n",server_item->pid, rpc_id);
				continue;
			}
		}
	}

	ret = client_test_set_data(&binder_devp->binder_super, rpc_arr, 4, 12357);
	if (ret != BIND_RET_SUCCESS) {
		printk("set data error\n");
		return -EINVAL;
	}

	ret = client_req_server(binder_devp, &client_req);
	if (ret != BIND_RET_SUCCESS) {
		printk("req data error\n");
		return -EINVAL;
	}

    ret = client_test_set_data(&binder_devp->binder_super, rpc_arr, 4, 12457);
	if (ret != BIND_RET_SUCCESS) {
		printk("set data error\n");
		return -EINVAL;
	}

	ret = client_req_server(binder_devp, &client_req);
	if (ret != BIND_RET_SUCCESS) {
		printk("req data error\n");
		return -EINVAL;
	}

	printk("====================check client data begin=============================\n");

	//check_client_data(&binder_super, client_head, 12357);
    check_client_data_all(&binder_devp->binder_super, client_head);

	printk("====================check server data begin=============================\n");
	//check_server_data(&binder_super, server_head, 101);
	//check_server_data(&binder_super, server_head, 100);
	//check_server_data(&binder_super, server_head, 103);
    check_server_data_all(&binder_devp->binder_super, server_head);

	return 0;
}


#endif