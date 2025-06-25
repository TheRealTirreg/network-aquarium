import java.io.File;
import java.util.ArrayList;
import java.util.List;

import javafx.scene.image.Image;
import javafx.scene.image.ImageView;
import javafx.scene.layout.Pane;

/**
 * La classe Poisson représente un poisson dans l'interface graphique
 * <ul>
 *  <li> Elle hérite de la classe Pane pour pouvoir être ajoutée à un conteneur
 *  <li> Elle contient des attributs pour la position, la taille et l'image du poisson
 * </ul>
 * 
 * @param fishName Nom du poisson
 * @param current_x Position actuelle en x
 * @param current_y Position actuelle en y
 * @param width Largeur de l'image
 * @param height Hauteur de l'image
 * @param target_x Position cible en x
 * @param target_y Position cible en y
 * @param timeToTarget Temps pour atteindre la cible
 * @param moveX Déplacement en x au prorata de la temps
 * @param moveY Déplacement en y au prorata de la temps
 * <ul>
 */
public class Fish {
    private final Pane view; // used for rendering
    private final ImageView imageView;
    private double viewWidth, viewHeight;

    private final String name;
    private double currentX, currentY;
    private double targetX, targetY;
    private double moveX, moveY;
    private double timeToTarget;

    // Getter
    public String getName() {return this.name;}
    public double getCurrentX() {return this.currentX;}
    public double getCurrentY() {return this.currentY;}
    public double getMoveX() {return this.moveX;}
    public double getMoveY() {return this.moveY;}
    public double getTimeToTarget() {return this.timeToTarget;}
    public Pane getView() {return this.view;}

    // Setter
    public void setCurrentX(double currentX) {this.currentX = currentX;}
    public void setCurrentY(double currentY) {this.currentY = currentY;}

    // Constant dictionary mapping le nom de poissons à image paths
    private static final String[] fishNames = {
        "PoissonClown", "PoissonDiscus", "PoissonPapillon", "PoissonBarbier",
        "PoissonChirurgienJaune", "PoissonChirurgienMasque", "PoissonFraiseVanille",
        "PoissonIdoleMaure", "PoissonPapillonRoyal"
    };

    /**
     * Constructeur de la classe Poisson
     * @param fishName Nom du poisson
     * @param current_x Position actuelle en x
     * @param current_y Position actuelle en y
     * @param width Largeur de l'image
     * @param height Hauteur de l'image
     * @param target_x Position cible en x
     * @param target_y Position cible en y
     * @param timeToTarget Temps pour atteindre la cible
     */
    public Fish(String name, double currentX, double currentY, double width, double height, double viewWidth, double viewHeight, int fishId) {
        this.name = name;
        this.currentX = currentX;
        this.currentY = currentY;
        this.targetX = currentX;
        this.targetY = currentY;
        this.viewWidth = viewWidth;
        this.viewHeight = viewHeight;

        this.timeToTarget = 0;
        this.moveX = 0;
        this.moveY = 0;

        imageView = new ImageView(new Image(getImagePath(name, fishId)));
        imageView.setFitWidth(height);
        imageView.setFitHeight(width);

        view = new Pane(imageView);
    }

    public double getTargetX() { return targetX; }
    public double getTargetY() { return targetY; }
    public double getViewWidth() { return viewWidth; }
    public double getViewHeight() { return viewHeight; }
    public double getWidth() { return imageView.getFitWidth(); }
    public double getHeight() { return imageView.getFitHeight(); }

    /**
     * Imprime le nom du poisson et sa position actuel
     * @return nom, position actuel
     */
    @Override
    public String toString() {
        return "Poisson{" +
                "name='" + getName() + '\'' +
                ", currentX=" + getCurrentX() +
                ", currentY=" + getCurrentY() +
                '}';
    }

    /**
     * Cherche le chemin de l'image du poisson en fonction de son nom
     * @param fishName
     * @return le chemin de l'image
     */
    private static String getImagePath(String fishName, int fishId) {
        // Check if the prefix of the fish name (e.g., "poisson12" => "poisson") is present in the fish names list
        for (String fish : fishNames) {
            if (fishName.startsWith(fish)) {
                String prefix = fishName.substring(0, fish.length());
                return "file:img/" + prefix + ".png";
            }
        }
        // Return default image path if no match found
        String selectedFishName = fishNames[fishId % fishNames.length];
        ConsolePrinter.println("Image not found for fish name: " + fishName + ", using default: " + selectedFishName + " with id " + fishId);
        return "file:img/" + selectedFishName + ".png";
    }

    /**
     * Sets the vector to the target position
     */
    public void setTargetPosition(double targetX, double targetY, double timeToTarget, int framesPerSecond) {
        if (timeToTarget == 0) {
            timeToTarget = 1;  // Avoid division by zero
        }

        this.targetX = targetX;
        this.targetY = targetY;
        this.timeToTarget = timeToTarget;

        this.moveX = (targetX - currentX) / (timeToTarget * framesPerSecond);
        this.moveY = (targetY - currentY) / (timeToTarget * framesPerSecond);
        ConsolePrinter.println("Fish " + name + " => (" + targetX + ", " + targetY + ") in " + timeToTarget + "s");
    }

    public boolean draw(Pane root) {
        // Compute the angle in degrees from the movement vector
        double angleRad = Math.atan2(moveY, moveX);
        double angleDeg = Math.toDegrees(angleRad);

        // Flip the image horizontally if moving left
        if (moveX < 0) {
            imageView.setScaleX(-1);
            imageView.setRotate((180 + angleDeg) % 360);
        } else {
            imageView.setScaleX(1);
            imageView.setRotate(angleDeg);
        }

        // Calculate if the fish is within the view bounds
        // double fishWidth = imageView.getFitWidth();
        // double fishHeight = imageView.getFitHeight();
        // boolean isVisible = !(currentX + fishWidth < 0 || currentX > viewWidth ||
        //                     currentY + fishHeight < 0 || currentY > viewHeight);
        // if (!isVisible) {
        //     return false;
        // }

        // Move the fish to its new position
        view.setTranslateX(currentX);
        view.setTranslateY(currentY);
        if (!root.getChildren().contains(view)) {
            root.getChildren().add(view);
        }
        return true;
    }

    public void move() {
        currentX += moveX;
        currentY += moveY;
    }
}
