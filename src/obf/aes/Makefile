OBJ=aes256_dec aes256_enc obfuscate

%.o:%.c
	 g++ -c $(CFLAGS) $(CPPFLAGS) $< -o $@

obfuscate:aes256_enc.o aes256_dec.o

all:obfuscate
