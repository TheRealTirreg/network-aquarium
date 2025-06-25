import java.util.Scanner;
import java.io.File;
import java.io.FileNotFoundException;
import java.util.List;
import java.util.ArrayList;
import java.util.Arrays;


class readconfig {

    //Class variables from config file
    String controllerAdress;
    String id;
    int controllerPort;
    int displayTimeoutValue;
    String resources;

    //Additonal class variables
    private String configName = "affichage.cfg";

    //Getter


    public String getControllerAdress() {
        return controllerAdress;
    }

    public String getId() {
        return id;
    }

    public int getControllerPort() {
        return controllerPort;
    }

    public int getDisplayTimeoutValue() {
        return displayTimeoutValue;
    }

    public String getResources() {
        return resources;
    }

    //Constructor vide
    public readconfig() {
    }


    //Reads the config file
    public void readFile() {
        try {
            // Get the current working directory (project path)
            String projectPath = System.getProperty("user.dir");

            // Construct the full path to the config file by going up one folder
            String fullPath = projectPath + File.separator + "affichage.cfg";

            // Create a File object using the full path
            File file = new File(fullPath);
            Scanner sc = new Scanner(file);

            // Read the file line by line
            while (sc.hasNextLine()) {
                String line = sc.nextLine().trim();  // Remove leading/trailing spaces

                // Skip empty lines or comment lines
                if (line.isEmpty() || line.startsWith("#")) {
                    continue;
                }

                // Split the line into key and value using the '=' sign
                String[] parts = line.split("=", 2);

                if (parts.length == 2) {
                    String key = parts[0].trim(); // Key
                    String value = parts[1].trim(); // Value

                    
                    // Check the key and assign the corresponding value
                    switch (key) {
                        case "controller-address":
                            this.controllerAdress = value;
                            break;
                        case "id":
                            this.id = value;
                            break;
                        case "controller-port":
                            this.controllerPort = Integer.parseInt(value);
                            break;
                        case "display-timeout-value":
                            try {
                                this.displayTimeoutValue = Integer.parseInt(value);
                            } catch (NumberFormatException e) {
                                ConsolePrinter.error("Invalid number format for display-timeout-value");
                            }
                            break;
                        case "resources":
                            this.resources = value;
                            break;
                        default:
                            ConsolePrinter.error("Unknown key: " + key);
                            break;
                    }
                }
            }
            sc.close();
        }
        catch(FileNotFoundException e){
                ConsolePrinter.error("File not found");
            }
        }

    @java.lang.Override
    public java.lang.String toString() {
        return "readconfig{" +
                "controllerAdress='" + controllerAdress + '\'' +
                ", id='" + id + '\'' +
                ", controllerPort='" + controllerPort + '\'' +
                ", displayTimeoutValue=" + displayTimeoutValue +
                ", resources='" + resources + '\'' +
                ", configName='" + configName + '\'' +
                '}';
    }
}
