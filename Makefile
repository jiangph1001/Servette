objects =  servette.o
headerfile = response.h http_header_utils.h config.h
LIBS = -lpthread
EVENT = -levent
all:servette clean
	@echo "finish make"
servette:$(objects)
	gcc -o $@ $^ $(LIBS)
servette.o:$(headerfile) 
.PHONY:clean
clean:
	rm -fv $(objects)
event: servette_event.c
	gcc -o $@ $^ $(EVENT) $(LIBS)
test: tester.c
	gcc -o $@ $^ 