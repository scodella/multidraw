target=libmultidraw.so
src=$(wildcard src/*.cc)
obj=$(patsubst src/%.cc,obj/%.o,$(src))
inc=$(patsubst src/%.cc,inc/%.h,$(src))

$(target): $(obj) obj/dict.o
	mkdir -p lib
	g++ -std=c++17 -O2 -fPIC -shared $(shell root-config --libs) -o $@ $^

obj/dict.o: $(inc) src/LinkDef.h
	mkdir -p obj
	rootcling -f obj/dict.cc $^
	g++ -std=c++17 -O2 -fPIC -c -o $@ -I$(shell root-config --incdir) -I$(shell pwd) obj/dict.cc
	mkdir -p lib
	mv obj/dict_rdict.pcm lib/

obj/%.o: src/%.cc inc/%.h
	mkdir -p obj
	g++ -std=c++17 -O2 -fPIC -c -o $@ -I$(shell root-config --incdir) $<

.PHONY: clean

clean:
	rm -rf obj lib
