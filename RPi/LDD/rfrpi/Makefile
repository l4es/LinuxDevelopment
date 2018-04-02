CC 	= gcc
CX	= g++
OBJ_DIR = .
WPI_DIR = /tmp

INC = -I/usr/local/include -I .
TRFLAGS = -DTRACECORE433 -DTRACEEVENTMNG -DTRACESINGLETON 
NOTUSEDTRFLAGS = -DTRACE_RCOOK -DSENSORDEBUG 

CFLAGS = -c -MMD
LDFLAGS += -Xlinker --defsym -Xlinker RFRPI_BUILD_DATE=$$(date +'%Y%m%d') -L/usr/local/lib

rfrpi_dir = ./
rfrpi_files = ./RCSwitch.cpp ./RcOok.cpp ./Sensor.cpp ./core_433.cpp ./RCSwitch.cpp ./eventManager.cpp ./singleton.cpp 
rfrpi_files+= ./tools.cpp ./ledManager.cpp
rfrpi_objects=$(addsuffix .o,$(addprefix $(OBJ_DIR)/,$(basename $(notdir $(rfrpi_files))))) 

target_dir = .
target_files+= rfrpi_test.cpp
target_objects=$(addsuffix .o,$(addprefix $(OBJ_DIR)/,$(basename $(notdir $(target_files))))) 
target=./rfrpi_test


all:  $(target) 

$(target): $(target_objects) $(rfrpi_objects) 
	$(CX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@ -lwiringPi -lpthread 
	chmod +x $@

$(OBJ_DIR)/%.o: $(rfrpi_dir)/%.cpp $(rfrpi_dir)/%.h
	@echo '----------------------------------'
	@echo compiling $@
	$(CX) $(CFLAGS) $(INC) $< -o $@ $(TRFLAGS)
	@echo '-------------'
	
$(OBJ_DIR)/%.o: $(target_dir)/%.cpp
	@echo '----------------------------------'
	@echo compiling $@
	$(CX) $(CFLAGS) $(INC) $< -o $@ $(TRFLAGS)
	@echo '-------------'

clean:
	@rm $(OBJ_DIR)/*.o
	@rm $(target)
	