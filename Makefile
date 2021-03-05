TARGET := crunchy
CFLAGS := -std=c99 -pedantic-errors

$(TARGET): $(patsubst %.c,%.o,$(wildcard *.c))
	gcc -o $@ $^ $(LDFLAGS)

%.o : %.c
	gcc -o $@ $< -c $(CFLAGS)

clean :
	rm -f $(TARGET) $(wildcard *.o)

rebuild : clean $(TARGET)

.PHONY : clean rebuild
