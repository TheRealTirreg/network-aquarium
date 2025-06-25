import org.jline.reader.LineReader;

public class ConsolePrinter {
    private static LineReader reader;

    public static void init(LineReader r) {
        reader = r;
    }

    public static void println(String msg) {
        if (reader != null) {
            reader.printAbove(msg);
        } else {
            System.out.println(msg);
        }
    }

    public static void error(String msg) {
        println("[Erreur] " + msg);
    }
}
