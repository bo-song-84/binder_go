#include "stdio.h"
#include "sys/types.h"
#include "unistd.h"
#include "binder_app.h"

int read_content(int fd,  struct content_sendto_user *content)
{
    int ret;
    ret = read(fd, content, sizeof(*content));
    if (ret == -1) {
        printf("read address is %d;\n", content->map_size);
        return BIND_RET_ERROR;
    }
    return 0;
}

struct bind_percpu *cacu_percpu_address(const struct content_sendto_user *content, const void *map_address)
{
    uint offset = content->percpu_start_offset + content->per_cpu_size * content->cpu_id;
    
    return (struct bind_percpu *)(map_address + offset);
}

struct rpc_node *cacu_node_address(const struct content_sendto_user *content, const void *map_address, uint node_id)
{
    uint offset = content->node_start_offset + content->node_size * node_id;
    if (node_id == BIND_INVLIED_ID) return NULL;
    return (struct rpc_node *)(map_address + offset);
}

void *cacu_data_address(const struct content_sendto_user *content, const void *map_address, uint data_id)
{
    uint offset = content->data_start_offset + BINDER_BLOCK_SIZE * data_id;
    if (offset >= content->map_size) return NULL;

    return (void *)(map_address + offset);
}


