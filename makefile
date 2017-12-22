
MODULE_NAME=sskv
TARGET=$(MODULE_NAME)_mcd.so

CODE=..
TOP=../public
#YUN=..
CFLAGS+= -g -DSNACC_DEEP_COPY -DHAVE_VARIABLE_SIZED_AUTOMATIC_ARRAYS -fPIC -D_ENABLE_TNS_ -Wno-deprecated -O2

INC +=-I./proto -I$(CODE)/thirdparty/ -I$(CODE)/ -I$(CODE)/common/
INC +=-I$(TOP)/mcp++/inc -I$(TOP)/mcp++/mcp++/src/tns/inc
#INC +=-I$(YUN)/common/YiLicense/ -I$(YUN)/build64_release
INC +=-I$(CODE)/kv_store/kv_server/kv_module -I$(CODE)/kv_store/kv_core/

LIB +=-L$(TFC_WD)/. -lpthread \
		$(CODE)/build64_release/thirdparty/jsoncpp-0.6.0-dev/libjsoncpp.a \
		-L$(CODE)/public/mcp++/lib

LIB += $(CODE)/kv_store/kv_module/libkv_module.a
LIB += $(CODE)/public/mcp++/lib/libtfc.a

#LIB += $(CODE)/common/YiLicense/libYiLicense.a

SRC:=$(wildcard *.cpp)
OBJ:=$(patsubst %.cpp, %.o, $(SRC))
FLAGS:=-Wl,-rpath,/usr/local/lib/


all: $(OBJ)
	g++ $(CFLAGS) -shared -o $(TARGET) $(OBJ) $(INC) $(LIB) $(FLAGS)

${MODULE_PUBLIC}/%.o: ${MODULE_PUBLIC}/%.cpp
	$(CXX) $(CFLAGS) $(INC) -c -o $@ $<

${MODULE_PUBLIC}/%.o: ${MODULE_PUBLIC}/%.c
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CFLAGS) $(INC) -c -o $@ $<

clean:
	rm -rf *.o $(TARGET)
	rm -rf $(OBJ)
