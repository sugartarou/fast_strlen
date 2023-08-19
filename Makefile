all: mystrlen

FLAG=-march=native

mystrlen: mystrlen.c
	gcc -Ofast $(FLAG) $? -o $@_with_Ofast_native
	gcc -O2 $(FLAG) $? -o $@_with_O2_native
	gcc -O1 $(FLAG) $? -o $@_with_O1_native
	gcc $(FLAG) $? -o $@_with_native
	./mystrlen_with_Ofast_native
	./mystrlen_with_O2_native
	./mystrlen_with_O1_native
	./mystrlen_with_native
