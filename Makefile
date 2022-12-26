CC = g++ -std=c++11

INCLUDES = -I$(glad_inc) -I$(glm)
LIBRARIES = -lglfw3 \
-framework Cocoa \
-framework OpenGL \
-framework IOKit \
-framework CoreVideo

glad = /Users/macbook/Working/LearnOpenGL/LinkerGL/glad
glad_inc = $(glad)/include

glm = /Users/macbook/Working/LearnOpenGL/LinkerGL/glm

DEBUG  = -g
CFLAGS = -Wall -w -c $(DEBUG) $(INCLUDES)
LFLAGS = $(DEBUG) $(LIBRARIES)

cpp_files = main.cpp
OBJ = $(cpp_files:.cpp=.o) glad.o

app: $(OBJ)
	@echo "Building App!"
	$(CC) $(LFLAGS) -o $@  $(OBJ)
	@echo "Done!"

%.o: %.cpp
	$(CC) $(CFLAGS) $<

.PHONY : clean
clean:
	rm app
	rm *.o