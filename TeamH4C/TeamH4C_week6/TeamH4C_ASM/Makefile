# Makefile

FORMAT = elf64

mtable: mtable.o
	ld mtable.o -o mtable

mtable.o: mtable.asm
	nasm -f $(FORMAT) mtable.asm

clean:
	rm -f mtable.o mtable
