CC   = gcc

OBJ  = cruce.o cruce.h libcruce.a
LIBS =  
BIN  = cruce
CFLAGS = -g 
RM = rm -f

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o cruce $(LIBS)

clean: 
	${RM} $(OBJ) $(BIN)