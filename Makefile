DEBUG_OBJECTS = Debug/XTeam-InstaReap.o
RELEASE_OBJECTS = Release/XTeam-InstaReap.o
DEBUG_OUTPUT = Debug/XTeam-InstaReap.so
RELEASE_OUTPUT = Release/XTeam-InstaReap.so
INCLUDES = -I../../include
LIBRARY = /usr/lib/dmfc.so -lm
DEFINES = -D__LINUX__ -Imacros.h
CC = gcc -w
DCFLAGS = -fPIC -g
RCFLAGS = -fPIC -O

debug : debug_objects
	$(CC) -fpermissive -shared -ldl -g -o $(DEBUG_OUTPUT) $(DEBUG_OBJECTS) $(LIBRARY)

release : release_objects
	$(CC) -shared -ldl -O -o $(RELEASE_OUTPUT) $(RELEASE_OBJECTS) $(LIBRARY)

clean :
	rm *.o *~ Debug/*.o Debug/*~ Debug/*.so Release/*.o Release/*~ Release/*.so

debug_objects :
	$(CC) $(DCFLAGS) -c Anarchy.cpp -o Debug/XTeam-InstaReap.o $(INCLUDES) $(DEFINES)

release_objects :
	$(CC) $(RCFLAGS) -c Anarchy.cpp -o Release/XTeam-InstaReap.o $(INCLUDES) $(DEFINES)
