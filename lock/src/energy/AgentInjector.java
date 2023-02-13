package energy;

import com.sun.tools.attach.*;
import energy.agent.EnergyAgent;
import energy.util.MethodThrowingException;

import javax.tools.*;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.URI;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.List;
import java.util.jar.Attributes;
import java.util.jar.JarOutputStream;
import java.util.jar.Manifest;
import java.util.stream.Stream;
import java.util.zip.ZipEntry;

public class AgentInjector {
    private static final String AGENT_SOURCE_PATH = "src/energy/agent/EnergyAgent.java";
    private static final String AGENT_CLASS_ATTRIBUTE = "Agent-Class";
    private static final String AGENT_CLASS_NAME = EnergyAgent.class.getName();
    private static final String AGENT_JAR_NAME = "energy.jar";
    private static final String AGENT_JAR_PATH = "./" + AGENT_JAR_NAME;
    private static String targetClassName, targetPid;


    public static void main(String[] args) throws IOException {
        compileAgent();
        safeTryCatch(AgentInjector::generateJar);
        checkArgsForClassName(args);
        initClassName(args);
        initPid();
        safeTryCatch(AgentInjector::attachAgentToVM);
    }

    private static void checkArgsForClassName(String[] args) {
        if (args.length != 1) {
            throw new IllegalArgumentException("Need target class name to continue");
        }
    }

    private static void initClassName(String[] args) {
        targetClassName = args[0];
    }

    private static void initPid() {
        targetPid = VirtualMachine
                .list()
                .stream()
                .filter(AgentInjector::matchWithPid)
                .findFirst()
                .orElseThrow()
                .id();
    }

    private static boolean matchWithPid(VirtualMachineDescriptor vmd) {
        return vmd.displayName().startsWith(targetClassName);
    }

    private static void attachAgentToVM() throws
            AgentLoadException,
            AttachNotSupportedException,
            AgentInitializationException,
            IOException {
        VirtualMachine vm = null;
        try {
            vm = VirtualMachine.attach(targetPid);
            vm.loadAgent(AGENT_JAR_PATH);
        } finally {
            assert vm != null;
            vm.detach();
        }
    }

    private static void safeTryCatch(MethodThrowingException<Exception> method) {
        try {
            method.execute();
        } catch (AgentLoadException | AttachNotSupportedException | AgentInitializationException | IOException e) {
            throw new RuntimeException(e);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }


//    private static String generateManifestPath() throws IOException {
//        File file = new File();
//        File file = File.createTempFile(JAR_NAME, JAR_EXT);
//        file.deleteOnExit();
//        Charset cs = StandardCharsets.ISO_8859_1;
//        try (ZipOutputStream zos = new ZipOutputStream(new FileOutputStream(file))) {
//            zos.putNextEntry(new ZipEntry(MANIFEST_MF));
//            ByteBuffer bb = cs.encode(PREFIX_AGENT_CLASS_AND_NAME);
//            zos.write(bb.array(), bb.arrayOffset() + bb.position(), bb.remaining());
//            zos.write(10);
//            zos.closeEntry();
//        }
//        return file.getPath();
//    }

    private static void generateJar() throws IOException {
        FileOutputStream fos = new FileOutputStream(AGENT_JAR_NAME);
        JarOutputStream jos = new JarOutputStream(fos, generateManifest());
        jos.putNextEntry(new ZipEntry(AGENT_CLASS_NAME + ".class"));
        jos.closeEntry();
        jos.close();
        fos.close();

        System.out.println("[SUCCESS]: energy.jar created successfully.");
    }

    private static Manifest generateManifest() {
        Manifest manifest = new Manifest();
        Attributes attributes = manifest.getMainAttributes();
        attributes.put(Attributes.Name.MANIFEST_VERSION, "1.0");
        attributes.put(new Attributes.Name(AGENT_CLASS_ATTRIBUTE), AGENT_CLASS_NAME);

        System.out.println("[SUCCESS]: Manifest generated successfully.");
        return manifest;
    }

    private static void compileAgent() {
        JavaCompiler compiler = ToolProvider.getSystemJavaCompiler();
        StandardJavaFileManager sjfm = compiler.getStandardFileManager(null, null, null);
        String source = readAgentFromSource();
        JavaFileObject jfo = new JavaSourceFromString(AGENT_CLASS_NAME, source);

        JavaCompiler.CompilationTask ct = compiler.getTask(null,
                null,
                null,
                null,
                null,
                List.of(jfo));

        String message = ct.call() ?
                "[SUCCESS]: Agent compiled successfully." :
                "[FAIL]: Agent compilation failed.";
        System.out.println(message);
    }

    private static String readAgentFromSource() {
        StringBuilder sb = new StringBuilder();
        try (Stream<String> stream = Files.lines(Paths.get(AGENT_SOURCE_PATH))) {
            stream.forEach(t -> sb.append(t).append("\n"));
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
        return sb.toString();
    }

    private static class JavaSourceFromString extends SimpleJavaFileObject {
        private final String code;

        JavaSourceFromString(String name, String code) {
            super(URI.create("string:///" + name.replace(".", "/") + Kind.SOURCE.extension), Kind.SOURCE);
            this.code = code;
        }

        @Override
        public CharSequence getCharContent(boolean ignoreEncodingErrors) {
            return code;
        }
    }
}

//package org.mortbay.jetty.alpn.agent;
//
//        import static java.lang.System.getProperty;
//        import static java.lang.management.ManagementFactory.getRuntimeMXBean;
//        import static java.nio.charset.StandardCharsets.UTF_8;
//        import static java.nio.file.Files.createTempFile;
//        import static java.nio.file.Files.newOutputStream;
//        import static java.util.Arrays.asList;
//        import static java.util.jar.Attributes.Name.MANIFEST_VERSION;
//
//        import java.io.IOException;
//        import java.lang.reflect.InvocationTargetException;
//        import java.net.MalformedURLException;
//        import java.net.URL;
//        import java.net.URLClassLoader;
//        import java.nio.file.Files;
//        import java.nio.file.Path;
//        import java.nio.file.Paths;
//        import java.util.jar.JarOutputStream;
//        import java.util.jar.Manifest;

//public class DynamicAgent {
//    private static final String DYNAMIC_AGENT = "dynamic-agent";
//
//    public static void enableJettyAlpnAgent(String args) {
//        attachJavaAgent(Premain.class, args);
//    }
//
//    public static void attachJavaAgent(Class<?> agentClass, String args) {
//        Path toolsPath = resolveToolsPath();
//        String pid = resolvePid();
//        Path agentPath = createAgentProxy(agentClass);
//        Class<?> virtualMachineClass = loadVirtualMachineClass(toolsPath);
//        attachAgent(pid, agentPath, virtualMachineClass, args);
//    }
//
//    private static Path resolveToolsPath() {
//        Path javaHome = Paths.get(getProperty("java.home"));
//        for (Path libPath : asList(Paths.get("lib", "tools.jar"), Paths.get("..", "lib", "tools.jar"))) {
//            Path path = javaHome.resolve(libPath);
//            if (Files.exists(path)) {
//                return path;
//            }
//        }
//        throw new RuntimeException("Could not find tools.jar from java installation at " + javaHome);
//    }
//
//    private static String resolvePid() {
//        String nameOfRunningVM = getRuntimeMXBean().getName();
//        int p = nameOfRunningVM.indexOf('@');
//        if (p < 0) {
//            throw new RuntimeException("Could not parse pid from JVM name: " + nameOfRunningVM);
//        }
//        return nameOfRunningVM.substring(0, p);
//    }
//
//    private static Path createAgentProxy(Class<?> agentClass) {
//        try {
//            Path agentPath = createTempFile(DYNAMIC_AGENT, ".jar");
//            agentPath.toFile().deleteOnExit();
//            Manifest manifest = new Manifest();
//            manifest.getMainAttributes().put(MANIFEST_VERSION, "1.0");
//            manifest.getMainAttributes().putValue("Agent-Class", agentClass.getName());
//            try (JarOutputStream out = new JarOutputStream(newOutputStream(agentPath), manifest)) {
//                return agentPath;
//            }
//        } catch (IOException e) {
//            throw new RuntimeException("Could not create " + DYNAMIC_AGENT + ".jar", e);
//        }
//    }
//
//    private static Class<?> loadVirtualMachineClass(Path toolsPath) {
//        try {
//            URL[] urls = new URL[] { toolsPath.toUri().toURL() };
//            URLClassLoader loader = new URLClassLoader(urls, null);
//            return loader.loadClass("com.sun.tools.attach.VirtualMachine");
//        } catch (MalformedURLException|ClassNotFoundException e) {
//            throw new RuntimeException("Could not find code for dynamically attaching to JVM", e);
//        }
//    }
//
//    private static void attachAgent(String pid, Path agentPath, Class<?> virtualMachineClass, String args) {
//        try {
//            Object virtualMachine = virtualMachineClass.getMethod("attach", String.class).invoke(null, pid);
//            try {
//                Util.log("Dynamically attaching java agent");
//                virtualMachineClass.getMethod("loadAgent", String.class, String.class).invoke(virtualMachine, agentPath.toString(), args);
//            } finally {
//                virtualMachineClass.getMethod("detach").invoke(virtualMachine);
//            }
//        } catch (NoSuchMethodException|IllegalAccessException|InvocationTargetException e) {
//            throw new RuntimeException("Could not attach jetty-alpn-agent", e);
//        }
//    }
//
//    private DynamicAgent() { }
//}