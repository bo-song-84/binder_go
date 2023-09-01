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
//#include "binder_pro.h"
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
    {10101,rpc_call_test},
    {10102,rpc_call_test},
    {10103,rpc_call_test},
    {10104,rpc_call_test},
    {10105,rpc_call_test},
    {10106,rpc_call_test},
    {10107,rpc_call_test},
    {10108,rpc_call_test},
    {10109,rpc_call_test},
    {10110,rpc_call_test},
    {10111,rpc_call_test},
    {10112,rpc_call_test},
    {10113,rpc_call_test},
    {10114,rpc_call_test},
    {10115,rpc_call_test},
    {10116,rpc_call_test},
    {10117,rpc_call_test},
    {10118,rpc_call_test},
    {10119,rpc_call_test},
    {10120,rpc_call_test},
    {10121,rpc_call_test},
    {10122,rpc_call_test},
    {10123,rpc_call_test},
    {10124,rpc_call_test},
    {10125,rpc_call_test},
    {10126,rpc_call_test},
    {10127,rpc_call_test},
    {10128,rpc_call_test},
    {10129,rpc_call_test},
    {10130,rpc_call_test},
    {10131,rpc_call_test},
    {10132,rpc_call_test},
    {10133,rpc_call_test},
    {10134,rpc_call_test},
    {10135,rpc_call_test},
    {10136,rpc_call_test},
    {10137,rpc_call_test},
    {10138,rpc_call_test},
    {10139,rpc_call_test},
    {10140,rpc_call_test},
    {10141,rpc_call_test},
    {10142,rpc_call_test},
    {10143,rpc_call_test},
    {10144,rpc_call_test},
    {10145,rpc_call_test},
    {10146,rpc_call_test},
    {10147,rpc_call_test},
    {10148,rpc_call_test},
    {10149,rpc_call_test},
    {10150,rpc_call_test},
    {10151,rpc_call_test},
    {10152,rpc_call_test},
    {10153,rpc_call_test},
    {10154,rpc_call_test},
    {10155,rpc_call_test},
    {10156,rpc_call_test},
    {10157,rpc_call_test},
    {10158,rpc_call_test},
    {10159,rpc_call_test},
    {10160,rpc_call_test},
    {10161,rpc_call_test},
    {10162,rpc_call_test},
    {10163,rpc_call_test},
    {10164,rpc_call_test},
    {10165,rpc_call_test},
    {10166,rpc_call_test},
    {10167,rpc_call_test},
    {10168,rpc_call_test},
    {10169,rpc_call_test}
};


int main(void) {
    start_binder_server(rpc_calls, sizeof(rpc_calls)/sizeof(rpc_calls[0]));
    return 0;
}