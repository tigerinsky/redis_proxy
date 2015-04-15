#---------- env ----------
CXX=g++
CXXFLAGS=-D_GNU_SOURCE -D__STDC_LIMIT_MACROS -g -pipe -W -Wall -fPIC -fno-omit-frame-pointer
INCPATH=-I. -I/home/meihua/dy/src/redis_proxy/../glog/include -I/home/meihua/dy/src/redis_proxy/../protobuf/include -I/home/meihua/dy/src/redis_proxy/../hiredis/include -I/home/meihua/dy/src/redis_proxy/../gflags/include -I/home/meihua/dy/src/redis_proxy/../mysql-connector/include
LIBPATH=-Xlinker "-(" -ldl -lpthread -lm -lrt /home/meihua/dy/src/redis_proxy/../glog/lib/libglog.a /home/meihua/dy/src/redis_proxy/../protobuf/lib/libprotoc.a /home/meihua/dy/src/redis_proxy/../protobuf/lib/libprotobuf.a /home/meihua/dy/src/redis_proxy/../protobuf/lib/libprotobuf-lite.a /home/meihua/dy/src/redis_proxy/../hiredis/lib/libhiredis.a /home/meihua/dy/src/redis_proxy/../gflags/lib/libgflags.a /home/meihua/dy/src/redis_proxy/../gflags/lib/libgflags_nothreads.a /home/meihua/dy/src/redis_proxy/../mysql-connector/lib/libmysqlclient.a -Xlinker "-)"


#---------- phony ----------
.PHONY:all
all:prepare \
libredis_proxy.a \


.PHONY:prepare
prepare:
	mkdir -p ./output/lib ./output/include

.PHONY:clean
clean:
	rm -rf /home/meihua/dy/src/redis_proxy/redis_proxy.o ./output


#---------- link ----------
libredis_proxy.a:/home/meihua/dy/src/redis_proxy/redis_proxy.o \

	ar crs ./output/lib/libredis_proxy.a /home/meihua/dy/src/redis_proxy/redis_proxy.o
	cp /home/meihua/dy/src/redis_proxy/redis_proxy.h ./output/include/


#---------- obj ----------
/home/meihua/dy/src/redis_proxy/redis_proxy.o: /home/meihua/dy/src/redis_proxy/redis_proxy.cpp \
 /home/meihua/dy/src/redis_proxy/redis_proxy.h \
 /home/meihua/dy/src/redis_proxy/../hiredis/include/hiredis.h \
 /home/meihua/dy/src/redis_proxy/../hiredis/include/read.h \
 /home/meihua/dy/src/redis_proxy/../hiredis/include/sds.h \
 /home/meihua/dy/src/redis_proxy/../glog/include/glog/logging.h \
 /home/meihua/dy/src/redis_proxy/../gflags/include/gflags/gflags.h \
 /home/meihua/dy/src/redis_proxy/../gflags/include/gflags/gflags_declare.h \
 /home/meihua/dy/src/redis_proxy/../glog/include/glog/log_severity.h \
 /home/meihua/dy/src/redis_proxy/../glog/include/glog/vlog_is_on.h
	$(CXX) $(INCPATH) $(CXXFLAGS) -c -o /home/meihua/dy/src/redis_proxy/redis_proxy.o /home/meihua/dy/src/redis_proxy/redis_proxy.cpp


