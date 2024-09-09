APP = demo
SRCS = $(wildcard *.c)
HEADERS = $(wildcard *.h)
OBJS = api.obj

demo : ${SRCS} ${HEADERS}
	cc ${SRCS} -o ${APP}
	./demo

watch: 
	node watcher -v