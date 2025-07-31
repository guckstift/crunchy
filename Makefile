./build/crunchy: ./build/main.o ./build/print.o ./build/helpers.o ./build/lex.o ./build/parse.o ./build/analyse.o ./build/generate.o
	gcc -o $@ $^

./build/%.o: ./src/%.c ./src/crunchy.h
	gcc -c -o $@ $<

./build/generate.o: ./build/runtime.c.h

./build/runtime.c.h: ./src/runtime.c
	xxd -i > $@ < $^

clean:
	rm ./build/*.o
	rm ./build/*.c.h
	rm ./build/crunchy