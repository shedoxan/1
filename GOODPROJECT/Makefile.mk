CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lm

all: output1.png output3.png

output1.png: main.c
    $(CC) $(CFLAGS) -o main main.c $(LDFLAGS)
    ./main
    @echo "Output image saved as output1.png"

output3.png: main.c
    $(CC) $(CFLAGS) -o main main.c $(LDFLAGS)
    ./main
    @echo "Output image saved as output3.png"

clean:
    rm -f main output1.png output3.png