#include "binder_pro.h"
#include "binder_client_action.h"
#include "binder_malloc.h"
#include "binder_init.h"
#include "binder_map.h"
#include "binder_rb_tree.h"
#include "binder_server_action.h"

/*
struct binder_dev {
	struct binder_buffer_info binder_buffer_info;
	struct bind_super binder_super;
	struct bind_percpu binder_percpu;
	struct rb_root rpc_root;
	struct list_head  client_hash[BINDER_HASH_SIZE];
	struct list_head  server_hash[BINDER_HASH_SIZE];
	wait_queue_head_t r_client_rq;	
	wait_queue_head_t r_server_rq;
};
*/

struct binder_dev binder_dev;

static void list_hash_init(void)
{
	int i ;
	for (i = 0 ; i < sizeof(binder_dev.client_hash) / sizeof(binder_dev.client_hash[0]); i++) {
		INIT_LIST_HEAD(&binder_dev.client_hash[i]);
	}

	for (i = 0 ; i < sizeof(binder_dev.server_hash) / sizeof(binder_dev.server_hash[0]); i++) {
		INIT_LIST_HEAD(&binder_dev.server_hash[i]);
	}
}

static long bind_rpc_operate(struct binder_dev *binder_devp, unsigned int cmd, unsigned long __arg) {
	uint rpc_id;
	uint ret;
	bool bool_ret;
	struct rpc_register *rpc_reg;
	struct rb_root *rpc_rb_root = &binder_devp->rpc_root;

	ret = copy_from_user(&rpc_id, (void __user *)__arg, sizeof(uint));
	if (ret != 0) {
		return -EINVAL;
	}

	switch (cmd) {
		case RB_TREE_INSERT_CMD: {
			rpc_reg = kmalloc(sizeof(struct rpc_register), GFP_KERNEL);
			if (!rpc_reg) {
				return -EINVAL;
			}
			memset(rpc_reg, 0, sizeof(struct rpc_register));
			rpc_reg->rpc_id = rpc_id;
			rpc_reg->pid = current->pid;
			bool_ret = register_rpc(rpc_rb_root,rpc_reg);
			if (!bool_ret) {
				kfree(rpc_reg);
				return -EINVAL; 
			}
			ret = insert_rpc_hlist(binder_devp->server_hash, rpc_reg, rpc_reg->pid);
			if (ret != BIND_RET_SUCCESS) {
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
		default:
			return BIND_INVLIED_ID;
	}

	return BIND_RET_SUCCESS;
}

static long binder_pro_ioctl(struct file *file, unsigned int cmd, unsigned long __arg) {
	struct binder_dev *binder_devp = (struct binder_dev *)file->private_data;
	struct server_item *server_item;
	struct client_req_hash *client_req;
	uint ret;

	pid_t pid;

	ret = bind_rpc_operate(binder_devp, cmd, __arg);
	if (ret == BIND_RET_SUCCESS) {
		return ret;
	}

	switch (cmd) {
		case RPC_REQUEST_CMD:
			ret = client_req_server(binder_devp, &client_req);
			if (ret != BIND_RET_SUCCESS) {
				printk("client req server error\n");
				return -EINVAL;
			}
			return copy_to_user((void __user *)__arg, &client_req->start_req_node, sizeof(client_req->start_req_node));
			break;
		case RPC_CLIENT_DSP:
			printk("print client==========================\n");
			check_client_data_all(&binder_devp->binder_super, binder_devp->client_hash);
			break;
		case RPC_SERVER_DSP:
			printk("print server==========================\n");
			check_server_data_all(&binder_devp->binder_super, binder_devp->server_hash);
			break;
		case RPC_SERVER_WAIT:
			pid = current->pid;
			server_item = find_server_item(binder_devp->server_hash, pid);
			printk("server wait \n");
			ret = wait_event_interruptible(binder_devp->r_server_rq, server_item->start_node_id != BIND_INVLIED_ID); 
			if (ret) {
				return -EINVAL;
			} 
			pid = current->pid;
			server_item = find_server_item(binder_devp->server_hash, pid);
			printk("start node id %d, process node id %d, invlied id %d\n", server_item->start_node_id, server_item->process_node_id, BIND_INVLIED_ID);
			start_server_rpc(server_item);
			printk("2start node id %d, process node id %d, invlied id %d\n", server_item->start_node_id, server_item->process_node_id, BIND_INVLIED_ID);
			return copy_to_user((void __user *)__arg, &server_item->process_node_id, sizeof(server_item->process_node_id));
		
			printk("start services\n");
			break;
		case RPC_SERVER_COMPLETE:
			pid = current->pid;
			server_item = find_server_item(binder_devp->server_hash, pid);
			client_mark(binder_devp, server_item->process_node_id);
			break;

		case RPC_CLIENT_WAIT:
			pid = current->pid;
			client_req = find_req_data(binder_devp->client_hash, pid);
			printk("client wait \n");
			wake_up_interruptible(&binder_devp->r_server_rq);
			ret = wait_event_interruptible(binder_devp->r_client_rq, client_req->req_cnt == atomic_read(&client_req->req_done_num)); 
			if (ret) {
				return -EINVAL;
			}
			printk("end client wait \n");
			break;
		//case RPC_SERVER_SET:
		case RPC_CLIENT_COMPLETE:
			pid = current->pid;
			client_req = find_req_data(binder_devp->client_hash, pid);
			bind_node_free_id_chain(&binder_devp->binder_super, client_req->start_req_node);
			break;
		default:
			break;
	}

	return BIND_RET_SUCCESS;
}

#include "binder_combin_test.h"

static int binder_pro_release(struct inode *inode, struct file *filp) {
	delete_rpc_by_pid();
	return 0;
}
static ssize_t binder_pro_write(struct file *file, const char __user *buf, size_t count, loff_t *off) {
	//copy_from_user()
	return 0;
}

static int binder_pro_open(struct inode *inode, struct file *filp) {
	filp->private_data = &binder_dev;
	return 0;
}

static unsigned int binder_pro_poll(struct file *file, poll_table *wait) {
	return 0;
}

/*
struct content_sendto_user {
	uint map_size;
	uint cpu_id;
	uint node_size;
	uint node_start_offset;
	uint data_start_offset;
	uint per_cpu_size;
	uint percpu_start_offset;
};
*/

static ssize_t binder_pro_read(struct file *file, char *buf, size_t count, loff_t *ppos) {
	struct binder_dev *binder_devp = (struct binder_dev *)file->private_data;
	struct content_sendto_user content;

	content.map_size = binder_devp->binder_buffer_info.buffer_size;
	content.cpu_id = smp_processor_id();
	content.node_size = binder_devp->binder_super.per_node_size;
	content.node_start_offset = node_offset_start(&binder_devp->binder_super);
	content.data_start_offset = data_offset_start(&binder_devp->binder_super);
	content.per_cpu_size = binder_devp->binder_super.per_cpu_size;
	content.percpu_start_offset = 0;

	return copy_to_user(buf, &content, count);
}

static int binder_pro_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct binder_dev *binder_devp = (struct binder_dev *)file->private_data;
	unsigned long start = vma->vm_start;
	unsigned long size = PAGE_ALIGN(vma->vm_end - vma->vm_start);
 
	if (size > binder_devp->binder_buffer_info.buffer_size || !binder_devp->binder_buffer_info.pfn) {
		printk("error here\n");
		return -EINVAL;
	}

	return remap_pfn_range(vma, start, binder_devp->binder_buffer_info.pfn, size, PAGE_SHARED);
}

static const struct file_operations binder_pro_fops = {
	.owner =		THIS_MODULE,
	.read =			binder_pro_read,
	.open =			binder_pro_open,
	.write = 		binder_pro_write,
	.release =		binder_pro_release,
	.mmap = 		binder_pro_mmap,
	.poll = 		binder_pro_poll,
	.unlocked_ioctl=binder_pro_ioctl
};

static struct miscdevice misc_binder = {
	0,
	BINDER_NAME,
	&binder_pro_fops
};

bool test_funcs = true;

static int __init binder_pro_init(void) {
	init_binder_block(&binder_dev.binder_buffer_info);
	init_super_block(&binder_dev.binder_super,&binder_dev.binder_buffer_info);
	list_hash_init();
	init_waitqueue_head(&binder_dev.r_client_rq);
	init_waitqueue_head(&binder_dev.r_server_rq);

	misc_register(&misc_binder);

	if (test_funcs) {
		//bitmap_test();
		//init_test(&binder_dev.binder_super);
		//rpc_test_multy(&binder_dev.rpc_root);
		//bind_malloc_test(&binder_dev.binder_super);
		//server_reg_test(&binder_dev);
	}

	return 0;
}

static void __exit binder_pro_exit(void) {
	misc_deregister(&misc_binder);
	free_pages((unsigned long)binder_dev.binder_buffer_info.buffer_address, 3);
}

module_init(binder_pro_init);
module_exit(binder_pro_exit);

MODULE_AUTHOR("songbo");
MODULE_DESCRIPTION("binder pro, new way to work as binder");
MODULE_LICENSE("GPL");



