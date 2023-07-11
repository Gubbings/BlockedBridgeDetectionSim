FLAGS =
LDFLAGS =
GPP = g++

FLAGS += -DMAX_THREADS_POW2=512
FLAGS += -std=c++17 -gdwarf -lpthread

no_optimize=1
ifeq ($(no_optimize), 1)	
	FLAGS += -O0
	FLAGS += -fno-inline-functions -fno-inline-functions-called-once -fno-optimize-sibling-calls
	FLAGS += -fno-default-inline -fno-inline
	FLAGS += -fno-omit-frame-pointer
else
	# FLAGS += -O3
	FLAGS += -O2	
endif

bin_dir=bin

default:
	$(GPP) ./main.cpp -o $(bin_dir)/simulation.exe $(FLAGS) $(CFLAGS) $(LDFLAGS)