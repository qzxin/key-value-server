# key-value-server

##Build
make :          编译server服务端
make client :   编译client 客户端
make clean_data 清除数据文件
make clean      清除编译临时文件
##Run
### 首先运行 ./server
### 运行客户端 ./client 127.0.0.1 
    支持多客户端同时和服务端通信，可另开 Terminal 运行新的客户端
### 客户端命令
    get [key]            search a key from the server, get the key's value.
    put [key]:[value]    put or set a data node(key and value) into server.
    save_cache           save the server's cache data
    help                 print this help
    exit                 kill the client
    shutdown             shutdown the server


