import org.jline.reader.LineReader;
import org.jline.reader.LineReaderBuilder;
import org.jline.reader.UserInterruptException;
import org.jline.reader.EndOfFileException;
import org.jline.terminal.Terminal;
import org.jline.terminal.TerminalBuilder;

import javafx.application.Platform;

import java.util.Arrays;
import java.util.List;

public class Commandes implements Runnable {

    private volatile boolean running = true;
    private final CustomEventListener listener;
    private Terminal terminal;
    private LineReader reader;

    private final List<String> commands = Arrays.asList("status", "addFish", "delFish", "startFish", "exit");
    private final List<String> mobilityModels = Arrays.asList("RandomWayPoint"); // Extendable

    public Commandes(CustomEventListener listener) {
        this.listener = listener;
    }

    private void notifyListener(CustomEvent event) {
        listener.onCustomEvent(event);
    }

    private void printHelp() {
        ConsolePrinter.println("Commandes disponibles :");
        ConsolePrinter.println("  - status");
        ConsolePrinter.println("  - addFish <Nom> at <X>x<Y>,<L>x<H>,<Mobilité>");
        ConsolePrinter.println("  - delFish <Nom>");
        ConsolePrinter.println("  - startFish <Nom>");
        ConsolePrinter.println("  - exit");
        ConsolePrinter.println("  - help");
    }

    @Override
    public void run() {
        try {
            terminal = TerminalBuilder.builder().system(true).build();
            reader = LineReaderBuilder.builder().terminal(terminal).build();
            ConsolePrinter.init(reader);  // Can now be uses anywhere in the code
            Runtime.getRuntime().addShutdownHook(new Thread(Platform::exit));

            while (running) {
                String line;
                try {
                    line = reader.readLine("> ").trim();
                } catch (UserInterruptException e) {
                    ConsolePrinter.println("\nFermeture demandée. Bloub Bloub !");
                    running = false;
                    Platform.exit();
                    System.exit(0);

                    break;
                } catch (EndOfFileException e) {
                    break;
                }

                if (line.isEmpty()) continue;

                String[] parts = line.split("\\s+", 2);
                String command = parts[0];
                String arguments = parts.length > 1 ? parts[1].trim() : "";

                if (command.equals("exit")) {
                    ConsolePrinter.println("\nFermeture demandée. Bloub Bloub !");
                    running = false;
                    Platform.exit();
                    break;
                } else if (command.equals("help")) {
                    printHelp();
                } else if (!commands.contains(command)) {
                    ConsolePrinter.println("NOK: commande introuvable");
                    printHelp();
                } else {
                    boolean valid = switch (command) {
                        case "status" -> {
                            if (!arguments.isEmpty()) {
                                ConsolePrinter.println("NOK: commande 'status' ne prend pas d'arguments");
                                yield false;
                            }
                            yield true;
                        }
                        case "addFish" -> validateAddFish(arguments);
                        case "delFish", "startFish" -> validateOneArg(command, arguments);
                        default -> true;
                    };

                    if (valid) {
                        notifyListener(new CustomEvent(command, arguments, "Cmd"));
                    }
                }
            }

        } catch (Exception e) {
            System.err.println("Erreur lors de la lecture du message - " + e.getMessage());
        } finally {
            closeCommandes();
        }
    }

    private boolean validateAddFish(String args) {
        // Expected: <name> at <coord>,<size>,<mobility>
        String[] tokens = args.trim().split("\\s+");
        if (tokens.length != 3) {
            ConsolePrinter.println("NOK: commande invalide (format général incorrect)");
            ConsolePrinter.println("  - addFish <Nom> at <X>x<Y>,<L>x<H>,<Mobilité>");
            return false;
        }

        String name = tokens[0];
        String atKeyword = tokens[1];
        String tail = tokens[2];

        if (!atKeyword.equals("at")) {
            ConsolePrinter.println("NOK: commande invalide (mot-clé 'at' manquant)");
            return false;
        }

        // Ensure there are no spaces inside the tail section
        if (tail.contains(" ")) {
            ConsolePrinter.println("NOK: commande invalide (espaces non autorisés après 'at')");
            return false;
        }

        String[] tailParts = tail.split(",");
        if (tailParts.length != 3) {
            ConsolePrinter.println("NOK: commande invalide (coordonnées, taille ou mobilité manquantes)");
            return false;
        }

        String coord = tailParts[0];
        String size = tailParts[1];
        String mobility = tailParts[2];

        if (!isCoordValid(coord)) {
            ConsolePrinter.println("NOK: commande invalide (coordonnées)");
            return false;
        }

        if (!isSizeValid(size)) {
            ConsolePrinter.println("NOK: commande invalide (taille)");
            return false;
        }

        if (!mobilityModels.contains(mobility)) {
            ConsolePrinter.println("NOK: modèle de mobilité non supporté");
            return false;
        }

        return true;
    }

    private boolean validateOneArg(String command, String args) {
        if (args.isEmpty()) {
            System.out.printf("NOK: commande '%s' nécessite un nom de poisson\n", command);
            return false;
        }

        // DelFish/StartFish PoissonA => OK
        if (!args.matches("^[a-zA-Z0-9_\\-]+$")) {
            ConsolePrinter.println("NOK: nom de poisson invalide");
            return false;
        }

        return true;
    }

    private boolean isSizeValid(String size) {
        String[] parts = size.split("x");
        if (parts.length != 2) return false;
        try {
            int x = Integer.parseInt(parts[0]);
            int y = Integer.parseInt(parts[1]);
            return x >= 0 && x <= 500 && y >= 0 && y <= 500;
        } catch (NumberFormatException e) {
            return false;
        }
    }

    private boolean isCoordValid(String coord) {
        String[] parts = coord.split("x");
        if (parts.length != 2) return false;
        try {
            int x = Integer.parseInt(parts[0]);
            int y = Integer.parseInt(parts[1]);
            return x >= 0 && x <= 100 && y >= 0 && y <= 100;
        } catch (NumberFormatException e) {
            return false;
        }
    }

    public void closeCommandes() {
        try {
            if (terminal != null) terminal.close();
        } catch (Exception e) {
            System.err.println("Erreur lors de la fermeture du terminal - " + e.getMessage());
        }
    }
}
