COMMON_INCLUDE_DIRS += $(rootdir)/$(MODULE)/include         \
                       $(incdir)/libonutils                   \
                       $(incdir)/liboncommunication              \
                       $(incdir)/libonevent                   \
                       $(incdir)/libonplatform

COMMON_SRC_FILES := $(rootdir)/$(MODULE)/src/tcp_server.c

COMMON_INST_HEADER_DIRS += $(rootdir)/$(MODULE)/include
