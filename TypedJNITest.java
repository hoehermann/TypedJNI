public class TypedJNITest 
{
    public static void main (String[] args)
    {
        printHelloWorld();
    }

    public static void printHelloWorld()
    {
        System.out.println("Java says: Hello World!");
    }

    public static void printLong(long l)
    {
        System.out.println("Java says long: " + l);
    }

    public static void print2Long(long l1, long l2)
    {
        System.out.println("Java says 2 long: " + l1 + " " + l2);
    }

    public static void printString(String s)
    {
        System.out.println("Java says string: " + s);
    }
    
    public static int increment(int i)
    {
        return i+1;
    }
    
    private int counter = 0;
    
    public TypedJNITest() {
        System.out.println("A new Java object was created out of the void.");
    }
    public TypedJNITest(int i) {
        this.counter = i;
        System.out.println("A new " + this.counter + " int Java object was created.");
    }
    public TypedJNITest(String s) {
        this.counter = Integer.parseInt(s);
        System.out.println("A new " + this.counter + " string Java object was created.");
    }
    
    public void printCounter() {
        System.out.println("Java says counter is " + this.counter + ".");
    }
    
    public int incrementCounterBy(int i) {
        return this.counter += i;
    }
}
