# 设置编译器和编译选项
CC = gcc
CFLAGS = -Wall -Werror -fPIC

TARGET_SERVER2=binder_server2
OBJS_SERVER2=binder_go_server2_app.o

TARGET_SERVER=binder_server
OBJS_SERVER=binder_go_server_app.o

TARGET_CLIENT=binder_client
OBJS_CLIENT=binder_go_client_app.o

TARGET_BINDER_LIB = libbinder_go.so
OBJS_BINDER_LIB = binder_go_client.o binder_go_server.o binder_go_commom.o

all: $(TARGET_SERVER) $(TARGET_CLIENT) $(TARGET_SERVER2)

$(TARGET_SERVER2): $(TARGET_BINDER_LIB) $(OBJS_SERVER2) $(OBJS_BINDER_LIB)
	$(CC)  $(OBJS_SERVER2) -o $(TARGET_SERVER2) -L. -lbinder_go

$(TARGET_SERVER): $(TARGET_BINDER_LIB) $(OBJS_SERVER) $(OBJS_BINDER_LIB)
	$(CC)  $(OBJS_SERVER) -o $(TARGET_SERVER) -L. -lbinder_go

$(TARGET_CLIENT): $(TARGET_BINDER_LIB) $(OBJS_CLIENT) $(OBJS_BINDER_LIB)
	$(CC)  $(OBJS_CLIENT) -o $(TARGET_CLIENT) -L. -lbinder_go

$(OBJS_SERVER2):%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

$(OBJS_SERVER):%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

$(OBJS_CLIENT):%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

# 生成动态链接库
$(TARGET_BINDER_LIB): $(OBJS_BINDER_LIB)
	$(CC) -shared $(OBJS_BINDER_LIB) -o $(TARGET_BINDER_LIB)

# 编译目标文件
$(OBJS_BINDER_LIB):%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

# 清理生成的文件
clean:
	rm -f $(OBJS_BINDER_LIB) $(TARGET_BINDER_LIB) $(OBJS_CLIENT) $(TARGET_CLIENT) $(TARGET_SERVER) $(OBJS_SERVER) $(TARGET_SERVER2) $(OBJS_SERVER2)
