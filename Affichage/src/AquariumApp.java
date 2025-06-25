import javafx.application.Application;
import javafx.application.Platform;
import javafx.geometry.Rectangle2D;
import javafx.scene.Scene;
import javafx.scene.image.Image;
import javafx.scene.image.ImageView;
import javafx.scene.image.PixelReader;
import javafx.scene.image.PixelWriter;
import javafx.scene.image.WritableImage;
import javafx.scene.layout.Pane;
import javafx.stage.Screen;
import javafx.stage.Stage;
import javafx.util.Duration;
import javafx.animation.Timeline;
import javafx.animation.KeyFrame;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

/**
 * La classe AquariumApp est l'application principale qui gère l'affichage de
 * l'aquarium
 * <ul>
 * <li>Elle étend la classe Application de JavaFX
 * <li>Elle implémente l'interface CustomEventListener pour traiter les
 * événements personnalisés
 * </ul>
 */

public class AquariumApp extends Application implements CustomEventListener {

    // Screen
    private Pane root;
    private ImageView backgroundImageView;
    private Client client; // Le client pour communiquer avec le serveur

    // Thread pour le JavaFX et pout l'animation
    private FishUpdater fishUpdater;

    // Informations sur la vue de l'aquarium
    private String viewId;
    private int viewWidth;
    private int viewHeight;
    private int viewOffsetX;
    private int viewOffsetY;

    // Informations sur la taille de l'affichage (taille de l'écran si pas en mode "small")
    private double windowWidth;
    private double windowHeight;
    private boolean isSmallMode = false;  // In small mode, the window is resized to the view size

    // Liste des poissons
    private List<Fish> fishes;

    // Constant update interval
    private static final int FPS = 24; // 24 FPS

    @Override
    public void start(Stage primaryStage) {

        // Obtenir la taille de l'écran
        Rectangle2D screenBounds = Screen.getPrimary().getVisualBounds();
        double minX = screenBounds.getMinX();
        double minY = screenBounds.getMinY();
        windowWidth = screenBounds.getWidth();
        windowHeight = screenBounds.getHeight();

        // Vérifier si le mode (donné par la variable d'environnement SIZE_MODE) est "small"
        String mode = System.getProperty("SIZE_MODE", "default");
        int dummySize = 500;
        isSmallMode = mode.equals("small");
        ConsolePrinter.println("Mode : " + mode);
        if (isSmallMode) {
            windowWidth = dummySize;
            windowHeight = dummySize;
        }

        // Créer un conteneur pour l'image de fond
        this.root = new Pane();

        // Charger l'image de fond
        Image backgroundImage = new Image("file:img/background3.png");
        backgroundImageView = new ImageView(backgroundImage);
        backgroundImageView.setFitWidth(windowWidth);
        backgroundImageView.setFitHeight(windowHeight);

        // Configurer la fenêtre principale
        Scene scene = new Scene(root, windowWidth, windowHeight);
        primaryStage.setTitle("Aquarium avec JavaFX");
        primaryStage.setScene(scene);
        primaryStage.sizeToScene();
        primaryStage.show();

        // Initialisation liste de poissons
        this.fishes = new ArrayList<Fish>();

        // Lancer la gestion des commandes terminal dans un thread séparé
        Commandes commandes = new Commandes(this);
        Thread commandesThread = new Thread(commandes);
        commandesThread.setDaemon(true);
        commandesThread.start();

        // Lancer le FishUpdater toutes les 41 ms (24 FPS => 1000 ms / 24 = 41 ms)
        ScheduledExecutorService scheduler = Executors.newScheduledThreadPool(1);
        this.fishUpdater = new FishUpdater(fishes, root, backgroundImageView);
        int millis = 1000 / FPS;
        scheduler.scheduleAtFixedRate(fishUpdater, 0, millis, TimeUnit.MILLISECONDS);

        // Envoyer un "ping" toutes les 3 secondes
        Timeline pingTimeline = new Timeline(
                new KeyFrame(Duration.seconds(3), event -> {
                    if (client.isConnected()) {
                        client.sendMessage("ping " + Client.id);
                    }
                }));
        pingTimeline.setCycleCount(Timeline.INDEFINITE);
        pingTimeline.play();

        // Lancer le client dans un thread séparé pour ne pas bloquer l'UI
        client = new Client(this);
        Thread clientThread = new Thread(client);
        clientThread.setDaemon(true); // Permet d'arrêter le thread quand l'application se ferme
        clientThread.start();

        this.root.getChildren().add(backgroundImageView);
    }

    /**
     * Methode pour traiter les messages reçus du controleur
     * <ul>
     * <li>Case "Cmd" : Traite les commandes envoyées par le terminal (addFish,
     * delFish)
     * <li>Case "Tcp" : Traite les messages envoyés par le serveur (greeting, no,
     * list, bye)
     * </ul>
     * Le message de "list" appelle la méthode extractFishElements, qui construit le
     * poisson avec les arguments envoyes
     */
    @Override
    public void onCustomEvent(CustomEvent event) {
        // Traiter l'événement reçu
        Platform.runLater(() -> {
            String message = event.getMessage();
            String sender = event.getSender();
            String args = event.getArgs();
            boolean ignore_print = message.startsWith("pong") || message.startsWith("list");
            if (!ignore_print) {
                ConsolePrinter.println("Message reçu : " + message + " de " + sender + " avec les arguments : " + args);
            }
            switch (sender) {
                case "Cmd":
                    switch (message) {
                        case "addFish":
                        case "delFish":
                        case "startFish":
                            client.sendMessage(message + " " + args);
                            break;
                        case "status":
                            printLocalStatus();
                            break;
                        default:
                            ConsolePrinter.println("NOK: commande inconnue");
                            break;
                    }
                    break;

                case "Tcp":
                    switch (message) {
                        case "greeting":
                            handleGreeting(args);
                            break;
                        case "no":
                            if (args.equals("greeting")) {
                                ConsolePrinter.println("NOK : pas d'affichage disponible");
                            }
                            break;
                        case "list":
                            extractFishElements(args);
                            break;
                        case "bye":
                            if (args.equals("timeout")) {
                                ConsolePrinter.println("NOK : le serveur a fermé la connexion");
                            }
                            break;

                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }

        });
    }

    private void handleGreeting(String args) {
        // Exemple d'args : "N1 0x0+600+800" ou "N1 100x200+600+800"
        try {
            String[] parts = args.split(" "); // ViewName + Aquarium dimensions
            String[] aquariumDim = parts[1].split("\\+"); // "100x200" + "600" + "800"
            String[] aquariumOffset = aquariumDim[0].split("x"); // "100" + "200"

            viewId = String.valueOf(parts[0]);
            viewOffsetX = Integer.parseInt(aquariumOffset[0]);
            viewOffsetY = Integer.parseInt(aquariumOffset[1]);
            viewWidth = Integer.parseInt(aquariumDim[1]);
            viewHeight = Integer.parseInt(aquariumDim[2]);

            actualizeAquariumSize();

        } catch (Exception e) {
            ConsolePrinter.println("NOK: commande invalide (greetings) " + args);
        }
    }

    // Crop la zone de l'image correspondant à l'aquarium
    // en utilisant les dimensions et le décalage
    private void actualizeAquariumSize() {
        ConsolePrinter.println("Aquarium ID: " + viewId);
        // Charger l'image de fond
        Image newBackgroundImage = new Image("file:img/background3.png");
        int bgWidth = (int) newBackgroundImage.getWidth();
        int bgHeight = (int) newBackgroundImage.getHeight();

        // Créer un PixelReader pour lire les pixels de l'image originale
        PixelReader pixelReader = newBackgroundImage.getPixelReader();

        // Créer un WritableImage pour stocker la zone découpée
        WritableImage croppedImage = new WritableImage(viewWidth, viewHeight);
        PixelWriter pixelWriter = croppedImage.getPixelWriter();

        // Copier les pixels de la zone découpée dans le WritableImage
        for (int y = 0; y < viewHeight; y++) {
            for (int x = 0; x < viewWidth; x++) {
                int srcX = viewOffsetX + x;
                int srcY = viewOffsetY + y;
                int argb = pixelReader.getArgb(srcX % bgWidth, srcY % bgHeight);
                pixelWriter.setArgb(x, y, argb);
            }
        }

        // Mettre à jour l'ImageView avec la nouvelle image
        backgroundImageView.setImage(croppedImage);

        // Redimensionner l'image pour s'adapter à la taille de l'écran
        double effectiveWidth = isSmallMode ? viewWidth : windowWidth;
        double effectiveHeight = isSmallMode ? viewHeight : windowHeight;
        backgroundImageView.setFitWidth(effectiveWidth);
        backgroundImageView.setFitHeight(effectiveHeight);

        Platform.runLater(() -> {
            root.getChildren().clear();
            root.getChildren().add(backgroundImageView);

            Stage stage = (Stage)root.getScene().getWindow();
            stage.sizeToScene();
        });
    }

    /**
     * Méthode pour extraire les éléments de poisson de la chaîne d'entrée
     * @param input Chaîne d'entrée contenant les informations sur les poissons
     * <ul>
     * <li> Extrait les éléments de poisson de la chaîne d'entrée
     * <li> Met à jour la liste des poissons
     * </ul>
     * IMPORTANT: Le controlleur envoie une liste de poissons sous la forme:
     * ["PoissonClown2" at 52x52,50x150,0]
     * <ul>
     * <li> S'il y a un nouveaux cible ou le poisson est nouveau, 
     * <li> le controlleur va envoye un element avec un 0 au fin deux fois
     * <li> premier fois avec la position actuelle
     * <li> deuxieme fois avec la position cible
     */
    private void extractFishElements(String input) {
        List<String> elements = new ArrayList<>();

        // Define the pattern to match elements within the list
        Pattern pattern = Pattern.compile("\\[([^\\]]+)\\]");
        Matcher matcher = pattern.matcher(input);

        // Find matches and add them to the list
        while (matcher.find()) {
            String element = matcher.group(1).trim();
            elements.add(element);
        }

        boolean updateFishTargetFlag = false;
        Fish fishToUpdate = null;
        for (String element : elements) {
            String[] parts = element.split(" ");
            String[] fishInfoStrings = parts[2].split(",");
            String[] targeStrings = fishInfoStrings[0].split("x");
            String[] fishSizeStrings = fishInfoStrings[1].split("x");

            // Stripping the fish name: "poisson12" => "poisson12"
            String fishName = parts[0].substring(1, parts[0].length() - 1);

            // Convert percentage position to pixels relative to view size
            int percentX = Integer.parseInt(targeStrings[0]);
            int percentY = Integer.parseInt(targeStrings[1]);
            int targetX = (int)(percentX / 100.0 * windowWidth);
            int targetY = (int)(percentY / 100.0 * windowHeight);

            // Parse the fish size and target time
            int fishWidth = Integer.parseInt(fishSizeStrings[0]);
            int fishHeight = Integer.parseInt(fishSizeStrings[1]);
            int fishTargetTime = Integer.parseInt(fishInfoStrings[2]);  // Could be a long too?

            // The targetX and targetY are in percentage of the view size.
            // Example: Aquarium size is 1000x1000 (which is only known by the server)
            //          Our view is anchored at 200x200 and has size 500x500
            //          Our screen size is FullHD (1920x1080) => We want to set our window size to 500x500
            // 
            // Assume we get a target position of 50x50. (Top left corner of the fish bounding box)
            // This means we want to move the fish to 50% of the view size => 50% of 500x500 => 250x250
            // 
            // Now assume our fish has arrived and we thus get a new target position of 110x-10.
            // In Aquarium coordinates, this would correspond to position 150x750 in the aquarium, which is outside of our view.
            // Once the fish moves out of the view, we still need to update the current position, but we don't need to draw it.

            if (updateFishTargetFlag) {
                // We need to verify if the fish to update is the same as the one we are currently processing
                if (fishToUpdate != null && fishToUpdate.getName().equals(fishName)) {
                    // If fish already exists, we need to update its target position
                    fishToUpdate.setTargetPosition(targetX, targetY, fishTargetTime, FPS);
                    updateFishTargetFlag = false;
                    fishToUpdate = null;
                    continue;
                }
                else {
                    // If is not the same fish, we need to delete the fishToUpdate
                    fishes.remove(fishToUpdate);
                    fishToUpdate = null;
                    updateFishTargetFlag = false;
                    ConsolePrinter.println("Fish " + fishName + " not found in the list, removing it.");
                }
            }

            if (fishTargetTime == -1) {
                // If target time is negative, we need to delete the fish
                for (Fish fish : fishes) {
                    if (fish.getName().equals(fishName)) {
                        fishes.remove(fish);
                        ConsolePrinter.println("deleted Fish " + fishName);
                        break;
                    }
                }
                continue;
            }

            // Check if target time is 0 (In this case, the next list element will contain the same fish with it's target position and time)
            // Check if fish is in our fish list
            if (fishTargetTime == 0) {
                // If target time is 0, we need to set the target position
                for (Fish fish : fishes) {
                    if (fish.getName().equals(fishName)) {
                        updateFishTargetFlag = true;
                        fishToUpdate = fish;
                        break;
                    }
                }
                // If Fish does not exist yet, create a new one
                if (!updateFishTargetFlag) {
                    // If fish does not exist, we need to create a new one. Here, the current position is the target position
                    Fish newFish = new Fish(fishName, targetX, targetY, fishWidth, fishHeight, windowWidth, windowHeight, fishes.size());
                    fishes.add(newFish);
                    updateFishTargetFlag = true;
                    fishToUpdate = newFish;
                }
            }
        }
    }

    public void printLocalStatus() {
        ConsolePrinter.println("-> OK : Connecté au contrôleur, " + fishes.size() + " poissons trouvés");
        for (Fish fish : fishes) {
            int px = (int) ((fish.getCurrentX() / (double) windowWidth) * 100);
            int py = (int) ((fish.getCurrentY() / (double) windowHeight) * 100);
            int pw = (int) ((fish.getWidth() / (double) windowWidth) * 100);
            int ph = (int) ((fish.getHeight() / (double) windowHeight) * 100);

            ConsolePrinter.println("Fish " + fish.getName() + " at " + px + "x" + py + "," + pw + "x" + ph);
        }
    }

    @Override
    public void stop() throws Exception {
        super.stop();
        Platform.exit();
        System.exit(0);
    }

    public static void main(String[] args) {
        launch(args);
    }
}