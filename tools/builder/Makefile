CS = mcs
NAME = builder
BINARY = $(NAME).exe

OBJECTS = ABuilder.cs AssemblyInfo.cs Config.cs LinuxBuilder.cs Win32Builder.cs \
	Package.cs PkgCore.cs Main.cs

default: all

all:	$(OBJECTS)
	$(CS) $(OBJECTS) -out:$(BINARY)

clean:
	rm -rf $(BINARY)

