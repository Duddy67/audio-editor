SRC = main.cpp menu.cpp main_callbacks.cpp main_functions.cpp \
      audio_engine.cpp audio_track.cpp waveform.cpp dialogs/dialog.cpp \
      dialogs/new_file.cpp dialogs/settings.cpp

CXX = g++
CXXFLAGS = -Wall $(shell fltk-config --cxxflags)

# Include OpenGL libraries
LFLAGS = $(shell fltk-config --ldflags) -lfltk_gl -lGL -lGLU -lX11

OBJS = $(SRC:.cpp=.o)
DIR_OBJ = obj/
DIR_OBJS = $(addprefix $(DIR_OBJ), $(OBJS))

$(DIR_OBJ)%.o: %.cpp *.h
	$(CXX) $(CXXFLAGS) -c $(<) -o $(@)

EXE = audioEditor

all: $(EXE)

$(EXE): $(DIR_OBJS)
	$(CXX) -o $@ $^ $(LFLAGS)

depend:
	makedepend -- $(CXXFLAGS) -- $(SRC)

strip: $(EXE)
	strip --strip-all $(EXE)

clean:
	rm -f $(DIR_OBJS)
	rm -f $(EXE)
