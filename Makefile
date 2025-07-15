./build/crunchy: ./build/main.o ./build/print.o ./build/lex.o ./build/parse.o ./build/analyse.o ./build/generate.o
	gcc -o $@ $^

./build/%.o: ./src/%.c
	gcc -c -o $@ $^

clean:
	rm ./build/*.o
	rm ./build/crunchy