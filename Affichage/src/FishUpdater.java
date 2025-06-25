import java.util.List;
import javafx.animation.TranslateTransition;
import javafx.application.Platform;
import javafx.scene.layout.Pane;
import javafx.util.Duration;
import javafx.scene.shape.Rectangle;
import javafx.scene.paint.Color;
import javafx.scene.image.ImageView;

/**
 * La classe FishUpdater est responsable
 * <ul> 
 *  <li> de la mise à jour de la position des poissons
 *  <li> de la gestion de l'animation de leur mouvement
 *  <li> de l'ajout des poissons au Pane
 * </ul>
 */
public class FishUpdater implements Runnable {

    private Pane root;
    private final ImageView backgroundImageView;
    private List<Fish> poissons;

    /**
     * Constructeur de la classe FishUpdater
     * @param poissons Liste de poissons à mettre à jour
     * @param root Pane dans lequel les poissons seront affichés
     */
    public FishUpdater(List<Fish> poissons, Pane root, ImageView backgroundImageView) {
        this.root = root;
        this.backgroundImageView = backgroundImageView;
        this.poissons = poissons;
    }

    /**
     * Thread pour JavaFX
     * <ul>
     *  <li> Méthode run qui est exécutée lorsque le thread est démarré
     *  <li> Elle met à jour la position des poissons et anime leur mouvement
     * </ul>
     */
    @Override
    public void run() {
        Platform.runLater(() -> {
            // ================== LOGIC ==================
            // Move all fish
            for (Fish fish : poissons) {
                fish.move();
            }

            // ================== RENDERING ==================
            // Clear and re-add background
            root.getChildren().clear();
            root.getChildren().add(backgroundImageView);

            // Draw all fish
            for (Fish fish : poissons) {
                fish.draw(root);  // fish.draw() assumes it adds itself to `root`
            }
        });
    }
}
