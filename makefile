run-test: test-data test
	./test -q test-cases/OR-Library/assignp800.txt test-cases/OR-Library/assignopt.txt

test: c/assignment.c c/test-orlib.c
	$(CC) -std=c89 -Wall -o $@ $^

test-data: test-cases/OR-Library/assignp800.txt test-cases/OR-Library/assignopt.txt

test-cases/OR-Library/assignp800.txt:
	mkdir -p test-cases/OR-Library
	cd test-cases/OR-Library && curl -O http://people.brunel.ac.uk/~mastjjb/jeb/orlib/files/assignp800.gz && gunzip -c assignp800.gz > assignp800.txt

test-cases/OR-Library/assignopt.txt:
	mkdir -p test-cases/OR-Library
	cd test-cases/OR-Library && curl -O http://people.brunel.ac.uk/~mastjjb/jeb/orlib/files/assignopt.txt
