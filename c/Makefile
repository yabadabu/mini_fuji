APP = demo
SRCS = $(wildcard *.c)
OBJS = ${foreach c,${SRCS},}
DEPS = $(wildcard *.h)

OBJS_PATH=./objs
OBJS=$(foreach f,$(SRCS),$(OBJS_PATH)/$(notdir $(basename $(f))).o)

demo : ${OBJS} ${DEPS} 
	cc ${OBJS} -o ${APP}
	./${APP}

$(OBJS_PATH) :
	@echo Creating temporal folder $(OBJS_PATH)
	@mkdir -p $(OBJS_PATH)

$(OBJS_PATH)/%.o : %.c $(DEPS) | $(OBJS_PATH)
	@echo C $<
	@$(CC) -c $(CFLAGS) -o $@ $<

clean :
	rm ${APP}
	rm -rf ${OBJS_PATH}

watch: 
	node watcher -v

.phony : demo