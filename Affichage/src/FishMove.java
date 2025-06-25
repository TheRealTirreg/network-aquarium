public class FishMove {

    private String fishName;
    private double x;
    private double y;
    private double fishWidth;
    private double fishHeight;
    private double fishMoveDelay;

    public FishMove(String fishName, double x, double y, double fishWidth, double fishHeight, double fishMoveDelay) {
        this.fishName = fishName;
        this.x = x;
        this.y = y;
        this.fishWidth = fishWidth;
        this.fishHeight = fishHeight;
        this.fishMoveDelay = fishMoveDelay;
    }

    public String getFishName() {
        return fishName;
    }

    public double getX() {
        return x;
    }

    public double getY() {
        return y;
    }

    public double getFishWidth() {
        return fishWidth;
    }

    public double getFishHeight() {
        return fishHeight;
    }

    public double getFishMoveDelay() {
        return fishMoveDelay;
    }

    @Override
    public String toString() {
        return "FishMove{" +
                "fishName='" + fishName + '\'' +
                ", x=" + x +
                ", y=" + y +
                ", fishWidth=" + fishWidth +
                ", fishHeight=" + fishHeight +
                ", fishMoveDelay=" + fishMoveDelay +
                '}';
    }
}
