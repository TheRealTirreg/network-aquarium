# Variables
SIZE_MODE ?= default
JAVAC = javac
JAVA = java
JAVA_FLAGS = --module-path javafx-sdk/lib --add-modules javafx.controls,javafx.fxml
JAVA_RUN_FLAGS = $(JAVA_FLAGS) -DSIZE_MODE=$(SIZE_MODE)# -Dprism.order=sw -Dprism.verbose=true

SRC_DIR = src
BIN_DIR = bin
LIB_DIR = lib
SRC_FILES = $(wildcard $(SRC_DIR)/*.java)
CLASS_FILES = $(SRC_FILES:$(SRC_DIR)/%.java=$(BIN_DIR)/%.class)
MAIN_CLASS = AquariumApp

# Classpath
JARS = $(wildcard $(LIB_DIR)/*.jar)
CLASSPATH = $(BIN_DIR):$(subst $(space),:,$(JARS))
space = $(empty) $(empty)

# Cibles
all: compile run

small:
	$(MAKE) SIZE_MODE=small all

compile:
	mkdir -p $(BIN_DIR)
	$(JAVAC) $(JAVA_FLAGS) -cp "$(CLASSPATH)" -d $(BIN_DIR) $(SRC_FILES)

run:
	$(JAVA) $(JAVA_RUN_FLAGS) -cp "$(CLASSPATH)" $(MAIN_CLASS)

clean:
	rm -rf $(BIN_DIR)

.PHONY: all compile run clean