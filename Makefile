LABS= lab1ideN3246 libideN3246.so
ALLFLAGS=-Wno-unused-function -Wall -Wextra -Werror -O2 

.PHONY: all clean

all: $(LABS)
clean:
	rm -rf *.o $(LABS)

lab1ideN3246: lab1ideN3246.c plugin_api.h
	gcc $(ALLFLAGS) -o lab1ideN3246 lab1ideN3246.c -ldl
libideN3246.so: libideN3246.c plugin_api.h
	gcc $(ALLFLAGS) -shared -fPIC -o libideN3246.so libideN3246.c -ldl -lm


