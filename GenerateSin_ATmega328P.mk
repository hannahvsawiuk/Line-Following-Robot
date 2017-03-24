# Specify the compiler to use
CC= avr-gcc
# Specify the microcontroller
CPU=-mmcu=atmega328p
# C compiler options
COPT= -g -Os $(CPU)
# Object files to link
OBJS= GenerateSin_ATmega328P.o

# The default 'target' (output) is GenerateSin_ATmega328P.elf and 'depends' on
# the object files listed in the 'OBJS' assignment above.
# These object files are linked together to create GenerateSin_ATmega328P.elf.
# The linked file is converted to hex using program aver-objcopy.
GenerateSin_ATmega328P.elf: $(OBJS)
	avr-gcc $(CPU) -Wl,-Map,GenerateSin_ATmega328P.map $(OBJS) -o GenerateSin_ATmega328P.elf
	avr-objcopy -j .text -j .data -O ihex GenerateSin_ATmega328P.elf GenerateSin_ATmega328P.hex
	@echo done!

# The object file GenerateSin_ATmega328P.o depends on GenerateSin_ATmega328P.c. GenerateSin_ATmega328P.c is compiled
# to create GenerateSin_ATmega328P.o.
GenerateSin_ATmega328P.o: GenerateSin_ATmega328P.c
	avr-gcc $(COPT) -c GenerateSin_ATmega328P.c

# Target 'clean' is used to remove all object files and executables
# associated wit this project
clean:
	@del *.hex *.elf *.o 2> nul

# Target 'FlashLoad' is used to load the hex file to the microcontroller 
# using the flash loader.
FlashLoad:
	spi_atmega328 -CRYSTAL -p -v GenerateSin_ATmega328P.hex

# Phony targets can be added to show useful files in the file list of
# CrossIDE or execute arbitrary programs:
dummy: GenerateSin_ATmega328P.hex GenerateSin_ATmega328P.map
	@echo Hello dummy!
	
explorer:
	explorer .
	