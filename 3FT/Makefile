GCC = gcc217
#GCC = gcc217m

.PRECIOUS: %.o

all: ft_client

clean:
	rm -f ft_client meminfo*.out

clobber: clean
	rm -f dynarray.o path.o ft_client.o checkerFT.o nodeFTGood.o ftGood.o *~

ft_client: dynarray.o path.o checkerFT.o nodeFTGood.o ft.o ft_client.o
	$(GCC) -g $^ -o $@

dynarray.o: dynarray.c dynarray.h
	$(GCC) -g -c $<

path.o: path.c path.h
	$(GCC) -g -c $<

ft_client.o: ft_client.c ft.h a4def.h
	$(GCC) -g -c $<

checkerFT.o: checkerFT.c dynarray.h checkerFT.h nodeFT.h path.h a4def.h
	$(GCC) -g -c $<

nodeFTGood.o: nodeFTGood.c dynarray.h checkerFT.h nodeFT.h path.h a4def.h
	$(GCC) -g -c $<

ftGood.o: ftGood.c dynarray.h checkerFT.h nodeFT.h ft.h path.h a4def.h
	$(GCC) -g -c $<