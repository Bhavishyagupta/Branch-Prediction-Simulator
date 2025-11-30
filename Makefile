CC = g++
OPT = -O3
WARN = -Wall
CFLAGS = $(OPT) $(WARN) $(STD) $(INC) $(LIB)

SIM_SRC = sim_bp.cc
SIM_OBJ = sim_bp.o

all: sim
	@echo "my work is done here..."

sim: $(SIM_OBJ)
	$(CC) -o sim $(CFLAGS) $(SIM_OBJ) -lm
	@echo "-----------DONE WITH sim-----------"

.cc.o:
	$(CC) $(CFLAGS)  -c $*.cc

.cpp.o:
	$(CC) $(CFLAGS)  -c $*.cpp

clean:
	rm -f *.o sim

clobber:
	rm -f *.o
