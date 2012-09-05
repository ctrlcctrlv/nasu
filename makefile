CC := gcc
CFLAGS := -O2
LIBS := -lstdc++ -lsfml-system -lsfml-window -lsfml-graphics -lsfml-audio -lm
.PHONY : clean install uninstall
nasu : obj
	$(CC) $(LIBS) nasu.o -o nasu
obj:
	$(CC) $(CFLAGS) -c nasu.cpp -o nasu.o
clean:
	rm -f nasu.o
install:
	mkdir /opt/nasu
	mv ./nasu /opt/nasu
	cp ./gamedata/* /opt/nasu
	ln -s /opt/nasu/nasu /usr/bin/nasu
uninstall:
	rm -rf /opt/nasu
	rm -f /usr/bin/nasu
reinstall:
	rm -rf /opt/nasu
	rm -f /usr/bin/nasu
	mkdir /opt/nasu
	mv ./nasu /opt/nasu
	cp ./gamedata/* /opt/nasu
	ln -s /opt/nasu/nasu /usr/bin/nasu
