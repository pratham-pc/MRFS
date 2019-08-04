all: test_case_1 test_case_2 test_case_3 test_case_4

test_case_1: test_case_1.o myfs.o
	gcc -pthread test_case_1.o myfs.o -o test_case_1
test_case_2: test_case_2.o myfs.o
	gcc -pthread test_case_2.o myfs.o -o test_case_2
test_case_1.o: test_case_1.c
	gcc -pthread -c test_case_1.c 
test_case_3: test_case_3.o myfs.o
	gcc -pthread test_case_3.o myfs.o -o test_case_3
test_case_4: test_case_4.o myfs.o
	gcc -pthread test_case_4.o myfs.o -o test_case_4
test_case_2.o: test_case_2.c
	gcc -pthread -c test_case_2.c 
test_case_3.o: test_case_3.c
	gcc -pthread -c test_case_3.c 
test_case_4.o: test_case_4.c
	gcc -pthread -c test_case_4.c 
myfs.o: myfs.c myfs.h
	gcc -pthread -c myfs.c
clean:
	rm *.o mydump-40.backup test_case_1 test_case_2 test_case_3 test_case_4
