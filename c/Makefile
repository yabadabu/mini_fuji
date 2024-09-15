APP = demo
SRCS = $(wildcard *.c)
OBJS = ${foreach c,${SRCS},}
DEPS = $(wildcard *.h)

OBJS_PATH=./objs
OBJS=$(foreach f,$(SRCS),$(OBJS_PATH)/$(notdir $(basename $(f))).o)

AUTO_GEN_FILES = properties.h
#CFLAGS = -O2

demo : ${OBJS} ${DEPS} 
	cc ${OBJS} -o ${APP}
	./${APP}

properties.h : generate_props.js property_defs.js
	node generate_props.js

$(OBJS_PATH) :
	@echo Creating temporal folder $(OBJS_PATH)
	@mkdir -p $(OBJS_PATH)

$(OBJS_PATH)/%.o : %.c $(DEPS) | $(OBJS_PATH) ${AUTO_GEN_FILES}
	@echo C $<
	@$(CC) -c $(CFLAGS) -o $@ $<

clean :
	rm -f ${APP}
	rm -rf ${OBJS_PATH}

watch: 
	node watcher -v

.phony : demo