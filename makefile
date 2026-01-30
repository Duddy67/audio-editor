# === Project sources ===
SRC = main.cpp menu.cpp main_callbacks.cpp main_functions.cpp \
      audio/audio_engine.cpp audio/audio_track.cpp view/waveform.cpp dialogs/dialog.cpp \
      dialogs/new_file.cpp dialogs/settings.cpp marking/marking.cpp \
      marking/marker.cpp dialogs/renaming.cpp widgets/time.cpp

# === Compiler setup ===
CXX = g++
CXXFLAGS = -Wall -MMD -MP $(shell fltk-config --cxxflags)
LFLAGS = $(shell fltk-config --ldflags) -lfltk_gl -lGL -lGLU -lX11

# === Directories ===
DIR_OBJ = obj/
OBJS = $(SRC:.cpp=.o)
DIR_OBJS = $(addprefix $(DIR_OBJ), $(OBJS))
DEPS = $(DIR_OBJS:.o=.d)

# === Target ===
EXE = audioEditor

# === Build rules ===
all: $(EXE)

$(EXE): $(DIR_OBJS)
	$(CXX) -o $@ $^ $(LFLAGS)

# Compile .cpp -> .o and generate .d dependency file
$(DIR_OBJ)%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# === Utilities ===
strip: $(EXE)
	strip --strip-all $(EXE)

clean:
	rm -f $(DIR_OBJS) $(DEPS) $(EXE)

# Include auto-generated dependency files if they exist
-include $(DEPS)
