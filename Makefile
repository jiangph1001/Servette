objects =  servette.o
file = filemanage
all:servette clean
	@echo "finish make"
servette:$(objects)
	gcc -o $@ $^
servette.o: $(file).h config.h
.PHONY:clean
clean:
	rm -fv $(objects)
