CC = gcc
IN = main.c src/main_state.c src/glad/glad.c
OUT = main.out
CFLAGS = -Wall -DGLFW_INCLUDE_NONE
LFLAGS = -lglfw3 -ldl -lm
IFLAGS = -I. -I./include -I./lib/GLFW/include/ -L./lib/GLFW/lib-arm64/
FFLAGS = -framework Cocoa -framework OpenGL -framework IOKit

.SILENT all: clean build run

clean:
	rm -f $(OUT)

build: $(IN) include/main_state.h include/stb_image.h 
	$(CC) $(IN) -o $(OUT) $(CFLAGS) $(LFLAGS) $(IFLAGS) $(FFLAGS)

run: $(OUT)
	./$(OUT)
