public class CustomEvent {

    // Attributs
    private final String message; // Le message de l'événement
    private final String args; // Les arguments de l'événement
    private final String sender; // L'expéditeur de l'événement

    public CustomEvent(String message, String args, String sender) {
        this.message = message;
        this.args = args;
        this.sender = sender;
    }

    public String getMessage() {
        return message;
    }

    public String getArgs() {
        return args;
    }

    public String getSender() {
        return sender;
    }

    
}
