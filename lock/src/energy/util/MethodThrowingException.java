package energy.util;

@FunctionalInterface
public interface MethodThrowingException<E extends Exception> {
    void execute() throws E;
}
