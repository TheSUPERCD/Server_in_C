CC = gcc
CFLAGS = -Wall -g
SRCDIR = server/src
OBJDIR = server/obj

CLIENTDIR = ./client

OBJ = $(OBJDIR)/kmeans.o $(OBJDIR)/matinverse.o $(OBJDIR)/server.o

all: clean server

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -c $< -o $@ $(CFLAGS)

server: $(OBJ)
	$(CC) $^ $(CFLAGS) -lpthread -o ./server/$@
	$(CC) $(CLIENTDIR)/client.c $(CFLAGS) -lreadline -o $(CLIENTDIR)/client

clean:
	rm -f $(OBJ) server/server $(CLIENTDIR)/client

cleanresfiles:
	rm -f $(CLIENTDIR)/results/*.txt server/intermediate_files/*.txt
