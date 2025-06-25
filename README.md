# Installation

## Affichage

### Installation JavaFX

- Go to https://gluonhq.com/products/javafx/
- Go to Downloads, then choose JavaFX version 21.0.x [LTS], OS, Architecture and Type=SDK
- Download
- Unzip and copy the folder `javafx-sdk/` into `Affichage/`

### Installation Jline

- Go to Affichage/
- `mkdir lib && cd lib`
- `curl -O https://repo1.maven.org/maven2/org/jline/jline/3.29.0/jline-3.29.0.jar`
- `curl -O https://repo1.maven.org/maven2/org/jline/jline-terminal/3.29.0/jline-terminal-3.29.0.jar`
- `curl -O https://repo1.maven.org/maven2/org/jline/jline-reader/3.29.0/jline-reader-3.29.0.jar`

## Contrôleur

### Installation ncurses

 - Install packages libncurses5-dev libncursesw5-dev

# Lancer les programmes

## Contrôleur

- `make`
- Then, load an aquarium saved in folder `Controleur/aquariums`

## Affichage

- `make` or `make small`

