# Compiler settings
CC = gcc
CFLAGS = -Wall -Werror -pthread

# Targets
all: wordcount

# Compile the threadpool library
threadpool.o: threadpool.c threadpool.h
	$(CC) $(CFLAGS) -c threadpool.c

# Compile the mapreduce library
mapreduce.o: mapreduce.c mapreduce.h threadpool.h
	$(CC) $(CFLAGS) -c mapreduce.c

# Compile the distributed word count example
distwc.o: distwc.c mapreduce.h
	$(CC) $(CFLAGS) -c distwc.c

# Link the object files to create the final executable (wordcount)
wordcount: threadpool.o mapreduce.o distwc.o
	$(CC) $(CFLAGS) -o wordcount threadpool.o mapreduce.o distwc.o

# Run the wordcount program with sample inputs
run: wordcount
	./wordcount sample_inputs/sample1.txt sample_inputs/sample2.txt sample_inputs/sample3.txt sample_inputs/sample4.txt sample_inputs/sample5.txt sample_inputs/sample6.txt sample_inputs/sample7.txt sample_inputs/sample8.txt sample_inputs/sample9.txt sample_inputs/sample10.txt sample_inputs/sample11.txt sample_inputs/sample12.txt sample_inputs/sample13.txt sample_inputs/sample14.txt sample_inputs/sample15.txt sample_inputs/sample16.txt sample_inputs/sample17.txt sample_inputs/sample18.txt sample_inputs/sample19.txt sample_inputs/sample20.txt

# Clean up object files and the executable
clean:
	rm -f *.o wordcount
	rm -rf result-*
