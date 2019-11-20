
all:
	g++ src/client2.cpp -o cli -lboost_system -lpthread
	g++ src/svr_async_st.cpp -o svr_async_st -lboost_system -lboost_thread
	./svr_async_st &
	./cli 127.0.0.1

clean:
	kill `ps|grep "\bsvr"|awk '{print $1}'`
	rm cli svr_async_st

