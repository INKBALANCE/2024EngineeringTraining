CC = gcc
CFLAGS = -Wall -pthread
LDFLAGS = -lrt

# 目标文件
TARGETS = producer consumer main

# 头文件
HEADERS = shared.h

# 默认目标
all: $(TARGETS)

# 生成目标文件
producer: producer.o shared.o
	$(CC) $(CFLAGS) -o producer producer.o shared.o $(LDFLAGS)

consumer: consumer.o shared.o
	$(CC) $(CFLAGS) -o consumer consumer.o shared.o $(LDFLAGS)

main: main.o shared.o
	$(CC) $(CFLAGS) -o main main.o shared.o $(LDFLAGS)

# 生成目标文件的依赖文件
producer.o: producer.c $(HEADERS)
	$(CC) $(CFLAGS) -c producer.c

consumer.o: consumer.c $(HEADERS)
	$(CC) $(CFLAGS) -c consumer.c

main.o: main.c $(HEADERS)
	$(CC) $(CFLAGS) -c main.c

shared.o: shared.c $(HEADERS)
	$(CC) $(CFLAGS) -c shared.c

# 清理目标文件和中间文件
clean:
	rm -f *.o $(TARGETS)

.PHONY: all clean

