package energy;

import energy.annotation.Energy;

import java.util.Arrays;

public class Lock {

    private final String[] arr = {
            "Sentence 1",
            "Sentence 2",
            "Sentence 3",
            "Sentence 4",
            "Sentence 5"
    };

    private void access_critical_section() {
        while (true) {
//            long tid = Thread.currentThread().getId();
//            String name = Thread.currentThread().getName();
//
//            System.out.println("tid = [" + tid + "] " + name + " executing ...") ;
//            synchronized (this) {
//                for (int i = 0; i < arr.length; i++) {
//                    arr[i] = arr[i].toLowerCase();
//                }
//            }
        }
    }


    @Energy
    public void anotherUselessFunction() {

    }
    @Energy
    public void someRandomFunction() {

    }
    @Energy
    public void createNThreads(int n) {
        Thread[] threads = new Thread[n];

        for (int i = 0; i < n; i++) {
            threads[i] = new Thread(
                    this::access_critical_section,
                    "java-lock" + i + "]");
        }
        Arrays.asList(threads).forEach(Thread::start);
    }

    public static void main(String[] args) {

        new Lock().createNThreads(4);
    }
}