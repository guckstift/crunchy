./build/crunchy: ./build/main.o ./build/print.o ./build/helpers.o ./build/lex.o ./build/parse.o ./build/analyse.o ./build/generate.o
	gcc -o $@ $^

./build/%.o: ./src/%.c ./include/crunchy.h
	gcc -c -I ./include -o $@ $<

#./build/generate.o: ./build/runtime.c.h

#./build/runtime.c.h: ./src/runtime.c
#	xxd -i > $@ < $^

./test: ./test.cr.c ./src/runtime.c
	gcc -I ./include -o $@ $^

./test.cr.c:  ./build/crunchy ./test.cr
	./build/crunchy ./test.cr

%: %.c

%.o: %.c

clean:
	rm ./build/*.o
	rm ./build/*.c.h
	rm ./build/crunchy

.PHONY: clean