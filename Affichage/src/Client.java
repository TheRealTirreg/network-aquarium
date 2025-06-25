import java.io.*;
import java.net.*;
import java.util.Arrays;
import java.util.List;

/**
 * La classe Client gère la connexion au serveur
 * <ul>
 * <li> Envoie des messages au serveur
 * <li> Écoute les messages du serveur
 * <li> Notifie un écouteur d'événements
 * </ul>
 */
public class Client implements Runnable {

	private readconfig rc;

	// Attributs pour la connexion au serveur
	private Socket socket;
	private BufferedReader inputReader;
	private PrintWriter outputWriter;

	// Thread d'écoute des messages du serveur
	private final CustomEventListener listener;
	
	/**
	 * Constructeur de la classe Client
	 * @param listener L'instance de l'écouteur d'événements
	 * <ul>
	 * <li> lire le fichier de configuration
	 * </ul>
	 */
	public Client(CustomEventListener listener) {
		this.listener = listener;
		rc = new readconfig();
		rc.readFile();
	}

	private void notifyListener(CustomEvent event) {
		listener.onCustomEvent(event);
	}

	// Attributs de configuration
	public static String host = "localhost";
	public static int port = 1234;
	public static String id = "id";
	public static int displayTimeoutValue = 0;
	public static String resources = "resources";

	@Override
	public void run() {
		try {
			// Récupérer les valeurs de configuration
			loadConfigValues();

			
			socket = new Socket(host, port);
			ConsolePrinter.println("SOCKET = " + socket);

			inputReader = new BufferedReader(new InputStreamReader(socket.getInputStream()));
			outputWriter = new PrintWriter(new BufferedWriter(new OutputStreamWriter(socket.getOutputStream())), true);

			// Envoyer un message d'initialisation
			sendMessage("hello\n");

			// Démarrer un thread pour écouter les messages du serveur
			Thread listenThread = new Thread(this::listenToServer);
			listenThread.start();

			// Envoyer les messages au serveur avec un petit délai
			List<String> messages = Arrays.asList(
				"getFishesContinuously\n",
				"addFish PoissonPapillon at 9x52, 197x196, RandomWayPoint\n",
				"addFish PoissonDiscus at 72x14, 104x103, RandomWayPoint\n",
				"addFish PoissonClown at 87x98, 100x96, RandomWayPoint\n",
				"addFish PoissonChirurgienJaune at 11x58, 138x138, RandomWayPoint\n",
				"addFish PoissonFraiseVanille at 10x83, 372x361, RandomWayPoint\n",
				"addFish PoissonBarbier at 78x43, 168x165, RandomWayPoint\n",
				"addFish PoissonChirurgienMasque at 65x66, 166x163, RandomWayPoint\n",
				"addFish PoissonIdoleMaure at 67x73, 160x144, RandomWayPoint\n",
				"addFish PoissonPapillonRoyal at 65x98, 110x101, RandomWayPoint\n",
				"addFish PoissonClown at 37x68, 55x53, RandomWayPoint\n",
				"addFish PoissonDiscus at 43x11, 156x149, RandomWayPoint\n",
				"addFish PoissonPapillon at 96x61, 122x118, RandomWayPoint\n",
				"addFish PoissonBarbier at 17x65, 193x175, RandomWayPoint\n",
				"addFish PoissonClown1 at 39x47, 72x65, RandomWayPoint\n",
				"addFish PoissonClown2 at 84x19, 44x42, RandomWayPoint\n",
				"addFish PoissonClown3 at 49x92, 60x56, RandomWayPoint\n",
				"addFish PoissonClown4 at 91x13, 72x67, RandomWayPoint\n",
				"addFish PoissonClown5 at 23x76, 43x42, RandomWayPoint\n",
				"addFish PoissonClown6 at 45x75, 41x40, RandomWayPoint\n",
				"addFish PoissonClown7 at 28x54, 46x43, RandomWayPoint\n",
				"addFish PoissonClown8 at 82x38, 30x29, RandomWayPoint\n",
				"addFish PoissonClown9 at 32x59, 36x33, RandomWayPoint\n",
				"startFish PoissonClown1\n",
				"startFish PoissonClown2\n",
				"startFish PoissonClown3\n",
				"startFish PoissonClown4\n",
				"startFish PoissonClown5\n",
				"startFish PoissonClown6\n",
				"startFish PoissonClown7\n",
				"startFish PoissonClown8\n",
				"startFish PoissonClown9\n",
				"startFish PoissonPapillon\n",
				"startFish PoissonDiscus\n",
				"startFish PoissonClown\n",
				"startFish PoissonChirurgienJaune\n",
				"startFish PoissonFraiseVanille\n",
				"startFish PoissonBarbier\n",
				"startFish PoissonChirurgienMasque\n",
				"startFish PoissonIdoleMaure\n",
				"startFish PoissonPapillonRoyal\n",
				"startFish PoissonClown\n",
				"startFish PoissonDiscus\n",
				"startFish PoissonPapillon\n",
				"startFish PoissonBarbier\n"
			);

			for (String message : messages) {
				Thread.sleep(100); // Attendre 100ms entre chaque message
				sendMessage(message);
			}

			// Attendre que le thread d'écoute termine avant de fermer la connexion
			listenThread.join(); // Cela bloque jusqu'à ce que listenToServer() soit terminé

		} catch (UnknownHostException e) {
			System.err.println("Erreur : Hôte inconnu - " + e.getMessage());
		} catch (IOException e) {
			System.err.println("Erreur d'entrée/sortie - " + e.getMessage());
		} catch (InterruptedException e) {
			System.err.println("Le thread d'écoute a été interrompu : " + e.getMessage());
		} finally {
			closeConnection(); // Maintenant on ferme la connexion une fois que le thread est terminé
		}
	}

	/**
	 * Méthode pour envoyer un message au serveur
	 * @param message Le message à envoyer
	 * <ul>
	 * <li> Vérifie si le PrintWriter n'est pas nul
	 * <li> Vérifie si le message n'est pas nul
	 * <li> Envoie le message au serveur
	 * </ul>
	 */
	public void sendMessage(String message) {
		if (outputWriter != null && message != null) {
			if (!message.startsWith("ping")) {
			ConsolePrinter.println("Envoi du message : " + message);
			}
			outputWriter.println(message);
			outputWriter.flush();
		}
	}

	// Méthode pour charger les valeurs de configuration
	private void loadConfigValues() {
		
		// Récupérer les valeurs de configuration
		Client.port = rc.getControllerPort();
		Client.host = rc.getControllerAdress();
		Client.id = rc.getId();
		Client.displayTimeoutValue = rc.getDisplayTimeoutValue();
		Client.resources = rc.getResources();
	}

	/**
	 * Méthode pour écouter les messages du serveur
	 * <ul>
	 * <li> Lit les messages du serveur
	 * <li> Traite les messages reçus
	 * <li> Notifie l'écouteur d'événements
	 * </ul>
	 */
	private void listenToServer() {
		List<String> msgs = Arrays.asList("greeting", "no", "list", "bye", "pong", "OK", "NOK");
		String message;
		try {
			ConsolePrinter.println("Démarrage de l'écoute des messages du serveur...");
			while ((message = inputReader.readLine()) != null) {
				// ConsolePrinter.println("Message reçu : " + message);
				String messageFirstWord = message.split(" ")[0];
				String messageRest = message.substring(messageFirstWord.length()).trim();

				if (message.equals("no greeting (no aquarium available)")) {
					ConsolePrinter.println("Aucun aquarium disponible.");
					closeConnection();
					break;
				}
	
				if (msgs.contains(messageFirstWord)) {
					notifyListener(new CustomEvent(messageFirstWord, messageRest, "Tcp"));
				} else {
					ConsolePrinter.println("NOK: commande introuvable: '" + message + "'");
				}
			}
			ConsolePrinter.println("Fin du flux atteinte, fermeture de la connexion.");
		} catch (IOException e) {
			System.err.println("Erreur lors de la lecture du message - " + e.getMessage());
		}
	}

	/**
	 * Vérifie si le client est connecté au serveur
	 * @return true si le client est connecté, false sinon
	 */
	public boolean isConnected() {
		return socket != null && !socket.isClosed();
	}

	/** 
	 * Méthode pour fermer proprement la connexion
	 * <ul>
	 * <li> Ferme le BufferedReader
	 * <li> Ferme le PrintWriter
	 * <li> Ferme le Socket
	 * <li> Affiche un message de déconnexion
	 */
	public void closeConnection() {
		try {
			if (inputReader != null)
				inputReader.close();
			if (outputWriter != null)
				outputWriter.close();
			if (socket != null)
				socket.close();
			ConsolePrinter.println("Client déconnecté.");

			

		} catch (IOException e) {
			System.err.println("Erreur lors de la fermeture : " + e.getMessage());
		}
	}

}