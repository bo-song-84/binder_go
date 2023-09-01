#include <sys/mman.h>
#include <sys/ioctl.h>
#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "stdbool.h"

#include "binder_app.h"

char output[200];

void* rpc_call_test(uint  rcp_id, const void *input, uint input_len)
{
    char *input_read;
    int ret;
    
    input_read = (char *)input;
    ret = snprintf(output, 200, "recieve data:%d,%s", rcp_id, input_read);
    if (ret <= 0) {
        return NULL;
    }

    return output;
}

struct binder_rpc_register rpc_calls[] = {
    {10001,rpc_call_test},
    {10002,rpc_call_test},
    {10003,rpc_call_test},
    {10004,rpc_call_test},
    {10005,rpc_call_test},
    {10006,rpc_call_test},
    {10007,rpc_call_test},
    {10008,rpc_call_test},
    {10009,rpc_call_test},
    {10010,rpc_call_test},
    {10011,rpc_call_test},
    {10012,rpc_call_test},
    {10013,rpc_call_test},
    {10014,rpc_call_test},
    {10015,rpc_call_test},
    {10016,rpc_call_test},
    {10017,rpc_call_test},
    {10018,rpc_call_test},
    {10019,rpc_call_test},
    {10020,rpc_call_test},
    {10021,rpc_call_test},
    {10022,rpc_call_test},
    {10023,rpc_call_test},
    {10024,rpc_call_test},
    {10025,rpc_call_test},
    {10026,rpc_call_test},
    {10027,rpc_call_test},
    {10028,rpc_call_test},
    {10029,rpc_call_test},
    {10030,rpc_call_test},
    {10031,rpc_call_test},
    {10032,rpc_call_test},
    {10033,rpc_call_test},
    {10034,rpc_call_test},
    {10035,rpc_call_test},
    {10036,rpc_call_test},
    {10037,rpc_call_test},
    {10038,rpc_call_test},
    {10039,rpc_call_test},
    {10040,rpc_call_test},
    {10041,rpc_call_test},
    {10042,rpc_call_test},
    {10043,rpc_call_test},
    {10044,rpc_call_test},
    {10045,rpc_call_test},
    {10046,rpc_call_test},
    {10047,rpc_call_test},
    {10048,rpc_call_test},
    {10049,rpc_call_test},
    {10050,rpc_call_test},
    {10051,rpc_call_test},
    {10052,rpc_call_test},
    {10053,rpc_call_test},
    {10054,rpc_call_test},
    {10055,rpc_call_test},
    {10056,rpc_call_test},
    {10057,rpc_call_test},
    {10058,rpc_call_test},
    {10059,rpc_call_test},
    {10060,rpc_call_test},
    {10061,rpc_call_test},
    {10062,rpc_call_test},
    {10063,rpc_call_test},
    {10064,rpc_call_test},
    {10065,rpc_call_test},
    {10066,rpc_call_test},
    {10067,rpc_call_test},
    {10068,rpc_call_test},
    {10069,rpc_call_test}
};

int main(void) {
    start_binder_server(rpc_calls, sizeof(rpc_calls)/sizeof(rpc_calls[0]));
    return 0;
}