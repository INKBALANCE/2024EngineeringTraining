使用select模型实现的一个TCP服务器：SelectServer
实现了：
1. 该selectserver 运行时通过命令行接收参数IP和PORT，例如./SelectServer 127.0.0.3 8888
2. 该SelectServer需要实时输出当前客户端连接数量，客户端类型以及客户端发过来的消息。
3. 分别实现TCP客户端代码可以连接该服务器，发送消息，当消息为quit时退出客户端。至少能保证同时3个客户端连接服务器的效果。
4. 使用select函数及相关API完成，不使用多线程/多进程/进程池等技术

运行格式如下：
./server(client) 127.0.0.1（IP地址） 8888（端口号）
