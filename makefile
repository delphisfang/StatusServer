
CODE=..
TOP=../public
YUN=..
LIB_HTTP_PATH=$(TOP)/mcp++/mcp++/src/libhttp/
CFLAGS+= -g -DSNACC_DEEP_COPY -DHAVE_VARIABLE_SIZED_AUTOMATIC_ARRAYS -fPIC -D_ENABLE_TNS_ -Wno-deprecated -O2
INC +=-I$(LIB_HTTP_PATH) -I./proto -I$(CODE)/thirdparty/ -I$(CODE)/ -I$(CODE)/common/ -I$(CODE)/longconn/protocol 
INC +=-I$(TOP)/mcp++/inc -I$(TOP)/mcp++/mcp++/src/tns/inc
INC +=-I$(YUN)/common/YiLicense/ -I$(YUN)/build64_release
INC +=-I$(CODE)/kv_store/kv_server/kv_module -I$(CODE)/kv_store/kv_core/ -I$(CODE)/kv_store/proto
LIB +=-L$(TFC_WD)/. $(LIB_HTTP_PATH)libhttpcxx.a -lpthread \
	  $(CODE)/build64_release/thirdparty/jsoncpp-0.6.0-dev/libjsoncpp.a \
	  -L$(CODE)/build64_release/common/crypto/symmetric/ -laes -lcrypto_with_length \
	  $(CODE)/build64_release/common/system/libcheck_error.a \
	  $(CODE)/build64_release/thirdparty/glog-0.3.2/src/libglog.a \
	  $(CODE)/build64_release/thirdparty/gflags-2.0/src/libgflags.a \
	  -lcrypto \
	  -L$(CODE)/public/mcp++/lib

LIB += $(CODE)/kv_store/kv_module/libkv_module.a 
LIB += $(CODE)/kv_store/target/libkvcommon_proto.a
LIB += $(CODE)/public/mcp++/lib/libtfc.a
#LIB += $(CODE)/common/YiLicense/libYiLicense.a
#LIB += $(CODE)/ks/code/lib/libsirius_core.a $(CODE)/ks/code/lib/libsirius_net.a
			  
#MCP_OBJ=$(TFC_CCD)/tfc_net_ipc_mq.o \
		$(TFC_OLD)/tfc_ipc_sv.o \
		$(TFC_OLD)/tfc_debug_log.o \
		$(TFC_MCD)/tfc_cache_proc.o \
	    $(TFC_MCD)/tfc_cache_access.o \
		$(TFC_OLD)/tfc_error.o \
		$(TFC_OLD)/tfc_stat.o \
		$(TFC_OLD)/tfc_thread_sync.o \
		$(TFC_BASE)/tfc_base_config_file.o \
		$(TFC_CCD)/tfc_net_open_shmalloc.o \
		$(TFC_WD)/tfc_base_watchdog_client.o\
		$(TFC_BASE)/tfc_base_fast_timer.o \
		$(TFC)/TsdBase/MemKeyCache.o \
		$(TFC)/TsdBase/Sem.o \
		$(TFC)/TsdBase/Base.o 


SRC:=$(wildcard *.cpp)
OBJ:=$(patsubst %.cpp, %.o, $(SRC))
FLAGS:=-Wl,-rpath,/usr/local/lib/


all: $(OBJ)
	g++ $(CFLAGS) -shared -o sskv_mcd.so $(OBJ) $(INC) $(LIB) $(FLAGS)

${MODULE_PUBLIC}/%.o: ${MODULE_PUBLIC}/%.cpp
	$(CXX) $(CFLAGS) $(INC) -c -o $@ $<

${MODULE_PUBLIC}/%.o: ${MODULE_PUBLIC}/%.c
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CFLAGS) $(INC) -c -o $@ $<

clean:
	rm -rf *.o sskv_mcd.so
	rm -rf $(OBJ)
