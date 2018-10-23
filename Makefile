target=libmultidraw.so
src=$(wildcard src/*.cc)
obj=$(patsubst src/%.cc,obj/%.o,$(src))
inc=$(patsubst src/%.cc,interface/%.h,$(src))

$(target): $(obj) obj/dict.o
	g++ -std=c++17 -O2 -fPIC -shared $(shell root-config --libs) -o $@ $^

obj/dict.o: $(inc) obj/LinkDef.h
	mkdir -p obj
	rootcling -f obj/dict.cc $^
	g++ -std=c++17 -O2 -fPIC -c -o $@ -I$(shell root-config --incdir) -I$(shell pwd) obj/dict.cc

obj/LinkDef.h:
	mkdir -p obj
	./mkLinkDef.py

obj/%.o: src/%.cc $(inc)
	mkdir -p obj
	g++ -std=c++17 -O2 -fPIC -c -o $@ -I$(shell root-config --incdir) $<

.PHONY: clean

clean:
	rm -rf obj