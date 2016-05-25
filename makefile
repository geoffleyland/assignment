CFLAGS=-std=c89 -g -O2 -Wall
ORLIB_URL=http://people.brunel.ac.uk/~mastjjb/jeb/orlib/files/

test-orlib: c/assignment.c c/test-orlib.c
	$(CC) $(CFLAGS) -o $@ $^

test: test-data test-orlib
	./test-orlib -q test-cases/OR-Library/assignp800.txt test-cases/OR-Library/assignopt.txt

test-data: test-cases/OR-Library/assignp800.txt test-cases/OR-Library/assignopt.txt

test-cases/OR-Library/assignp800.txt:
	mkdir -p test-cases/OR-Library
	cd test-cases/OR-Library && curl -O $(ORLIB_URL)assignp800.gz && gunzip -c assignp800.gz > assignp800.txt

test-cases/OR-Library/assignopt.txt:
	mkdir -p test-cases/OR-Library
	cd test-cases/OR-Library && curl -O $(ORLIB_URL)assignopt.txt

clean:
	rm test-orlib