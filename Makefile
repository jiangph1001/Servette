objects =  servette.o
file = filemanage
headerfile = response.h config.h http_header_utils.h filemanage.h
LIBS = -lpthread
all:servette clean
	@echo "finish make"
servette:$(objects)
	gcc -o $@ $^ $(LIBS)
servette.o: $(file).h $(headerfile)
.PHONY:clean
clean:
	rm -fv $(objects)
