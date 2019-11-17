objects =  servette.o
file = filemanage
sh = cp_html.sh
dir = /data/www/
all:servette clean
	@echo "finish make"
servette:$(objects)
	gcc -o $@ $^
servette.o: $(file).h config.h
.PHONY:clean
clean:
	rm -fv $(objects)
install:
	mkdir -vp $(dir)
	cp www/*.html $(dir)
	echo finish install
