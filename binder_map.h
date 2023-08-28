#include "binder_pro.h"

#ifndef BINDER_MAP_H
#define BINDER_MAP_H

////  bitmap start   ////////////////////////////////////////////////////////////////////////////////
/* 判断bit_idx位是否为1,若为1则返回true，否则返回false */
bool bitmap_scan_test(unsigned char *bitmap, uint bit_size, uint bit_idx) {
    uint byte_idx = bit_idx / 8;    // 向下取整用于索引数组下标
    uint bit_odd  = bit_idx % 8;    // 取余用于索引数组内的位

	if (bit_idx >= bit_size) return false;
    return (bitmap[byte_idx] & (BITMAP_MASK << bit_odd));
}

uint bitmap_scan(unsigned char *bitmap, uint bit_size, uint32_t cnt) {
    uint bit_continue_size;
    uint bit_odd;
    uint start_bit;
	uint bit_idx;
    
    bit_odd = 8;    /* default value is 8, for the last byte set the real bit_odd */
    bit_continue_size = 0;

	for(bit_idx = 0; bit_idx < bit_size; bit_idx++) {
        //value = (uint)(bitmap[bit_idx / 8] & (BITMAP_MASK << (bit_idx % 8)));
		if ((bitmap[bit_idx / 8] & (BITMAP_MASK << (bit_idx % 8))) == 0) {
			if (bit_continue_size == 0) {
				start_bit = bit_idx;
			}
			bit_continue_size++;
		} else {
			bit_continue_size = 0;
		}

		if (bit_continue_size == cnt) {
            printk("here return start_bit %d \n",start_bit);
			return start_bit;
		}
        //printk("bit_idx is %d, char value is %d, value is %d, start bit %d, bit size is  %d\n", bit_idx,bitmap[bit_idx/8], value, start_bit, bit_size);
	}

    return BIND_MAP_ERROR;
}

void bitmap_set_zero(unsigned char *bitmap, uint bit_idx) {
   uint32_t byte_idx = bit_idx / 8;    // 向下取整用于索引数组下标
   uint32_t bit_odd  = bit_idx % 8;    // 取余用于索引数组内的位

    bitmap[byte_idx] &= ~(BITMAP_MASK << bit_odd);
}

void bitmap_set_one(unsigned char *bitmap, uint bit_idx) {
   uint32_t byte_idx = bit_idx / 8;    // 向下取整用于索引数组下标
   uint32_t bit_odd  = bit_idx % 8;    // 取余用于索引数组内的位

   bitmap[byte_idx] |= (BITMAP_MASK << bit_odd);
}

uint bitmap_scan_set(unsigned char *bitmap, uint bit_size, uint32_t cnt, spinlock_t *lock , uint *bit_left) {
	unsigned long flags;
	uint i;
	uint idx;

	spin_lock_irqsave(lock, flags);	/* 上锁 */
    printk("get into the set one \n");
	idx = bitmap_scan(bitmap, bit_size, cnt);
	if (idx == BIND_MAP_ERROR) {
		printk("binder map error\n");
		spin_unlock_irqrestore(lock, flags);/* 解锁 */
        printk("go out the set one fail\n");
		return BIND_MAP_ERROR;
	}
	for (i = 0; i < cnt; i++) {
		bitmap_set_one(bitmap, idx + i);
	}
	*bit_left = *bit_left - cnt;
	spin_unlock_irqrestore(lock, flags);/* 解锁 */
    printk("go out the set one success\n");	
	return idx;
}

void bitmap_set_zero_lock(unsigned char *bitmap, uint bit_idx, spinlock_t *lock, uint *bit_left) {
	unsigned long flags;

	spin_lock_irqsave(lock, flags);	/* 上锁 */
    printk("get into the zero \n");
	bitmap_set_zero(bitmap, bit_idx);
	(*bit_left)++;
	spin_unlock_irqrestore(lock, flags);/* 解锁 */
    printk("go out the zero \n");
}

uint node_offset_start(struct bind_super *super) {
    uint offset;
    offset = super->per_cpu_size_total + super->node_map_size + super->data_map_size;
    return offset;
}

uint data_offset_start(struct bind_super *super) {
    uint offset;
    offset = super->per_cpu_size_total + super->node_map_size + super->data_map_size + super->node_size_total;
    return offset;
}

uint caculate_offset_by_node_id(struct bind_super *super, uint idx){
    uint offset;
    offset = node_offset_start(super) + idx * super->per_node_size;
    return offset;
}

uint caculate_offset_by_data_id(struct bind_super *super,uint idx){
    uint offset;
    offset = data_offset_start(super) + idx * BINDER_BLOCK_SIZE;
    return offset;
}

void *caculate_node_address(struct bind_super *super, uint idx) {
    void *address;
    if (idx >= super->node_num_total) return NULL;
    if (idx == BIND_INVLIED_ID) return NULL;
    address = super->binder_buffer_info->buffer_address + caculate_offset_by_node_id(super,idx);
    return address;
}

void *caculate_data_address(struct bind_super *super, uint idx) {
    void *address;
    address = super->binder_buffer_info->buffer_address + caculate_offset_by_data_id(super,idx);
    return address;
}




uint bitmap_scan_set_test(unsigned char *bitmap, uint bit_size, uint32_t cnt) {
	uint i;
	uint idx;

	idx = bitmap_scan(bitmap, bit_size, cnt);
	if (idx == BIND_MAP_ERROR) {
		printk("binder map error %d\n",idx);
		return BIND_MAP_ERROR;
	}
	for (i = 0; i < cnt; i++) {
		bitmap_set_one(bitmap, idx + i);
	}

    printk("start bit is %d\n", idx);
	
	return BIND_MAP_SUCCESS;
}


/*
void bitmap_set_zero(unsigned char *bitmap, uint bit_idx) {
   uint32_t byte_idx = bit_idx / 8;    // 向下取整用于索引数组下标
   uint32_t bit_odd  = bit_idx % 8;    // 取余用于索引数组内的位

    bitmap[byte_idx] &= ~(BITMAP_MASK << bit_odd);
}

void bitmap_set_one(unsigned char *bitmap, uint bit_idx) {
   uint32_t byte_idx = bit_idx / 8;    // 向下取整用于索引数组下标
   uint32_t bit_odd  = bit_idx % 8;    // 取余用于索引数组内的位

   bitmap[byte_idx] |= (BITMAP_MASK << bit_odd);
}
*/

void bitmap_test(void)
{
    unsigned long int test;

    //uint start_id;

    test = 100;
    printk("test value is %lx, size of test:%ld \n", test, sizeof(test));

    //start_id = bitmap_scan_set_test((unsigned char *)(&test), 8 * sizeof(test), 1);
    bitmap_set_one((unsigned char *)(&test),32);
    printk("test value is %lx, size of test:%ld \n", test, sizeof(test));
    bitmap_set_one((unsigned char *)(&test),0);
    printk("test value is %lx, size of test:%ld \n", test, sizeof(test));

    bitmap_set_zero((unsigned char *)(&test),0);
    printk("test value is %lx, size of test:%ld \n", test, sizeof(test));
    bitmap_set_zero((unsigned char *)(&test),32);
    //start_id = bitmap_scan_set_test((unsigned char *)(&test), 8 * sizeof(test), 4);
    printk("test value is %lx, size of test:%ld \n", test, sizeof(test));

}




////  bitmap end     ////////////////////////////////////////////////////////////////////////////////
#endif

