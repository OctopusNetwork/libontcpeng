rootdir := $(shell pwd)/../../..
project := ubuntu-base

include $(rootdir)/build/common/inc.mk
ifeq ($(hostmaketype), windows)
rootdir := ../../..
endif

####
##  Application targets
####
DEP_HEADERS += $(incdir)/libontcpeng $(incdir)/libonplatform    \
               $(incdir)/liboncommunication $(incdir)/libonevent

apps = mt_tcp_server

mt_tcp_server_SRCS = $(rootdir)/libontcpeng/test/mt/tcp_server_main.c
mt_tcp_server_ARCHIVES = libontcpeng libonplatform libonevent liboncommunication
mt_tcp_server_LDLIBS = -pthread
mt_tcp_server_DISTSOLIBS =

include $(rootdir)/build/utils/app.mk
