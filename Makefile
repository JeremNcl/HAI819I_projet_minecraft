# Variables
BUILD_DIR = build
EXECUTABLE = VoxelEngine
ARGS ?= 3

.PHONY: all configure build run clean distclean

# Commande par défaut : on compile juste
all: build

# 1. Générer la config (à faire manuellement la première fois ou après un distclean)
configure:
	mkdir -p $(BUILD_DIR)
	cmake -S . -B $(BUILD_DIR)

# 2. Compiler (lance configure automatiquement SI le dossier build n'existe pas)
build:
	@if [ ! -d "$(BUILD_DIR)" ]; then $(MAKE) configure; fi
	make -C $(BUILD_DIR)

# 3. Lancer le jeu (compile automatiquement avant si nécessaire)
run: build
	./$(BUILD_DIR)/$(EXECUTABLE) $(ARGS)

# 4. Nettoyage doux : supprime les binaires mais garde la config CMake
clean:
	make -C $(BUILD_DIR) clean

# 5. Nettoyage radical : rase tout la config (nécessitera de reconfigurer)
distclean:
	rm -rf $(BUILD_DIR)/*