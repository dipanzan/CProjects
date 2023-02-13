package energy;


import com.sun.tools.attach.VirtualMachine;
import com.sun.tools.attach.VirtualMachineDescriptor;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.lang.instrument.Instrumentation;
import java.lang.management.ManagementFactory;
import java.nio.ByteBuffer;
import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.util.UUID;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

public class SelfAttacher {
    public static Instrumentation BACK_LINK;

    public static void main(String[] args) throws Exception {
        // create a special property to verify our JVM connection
        String magic = UUID.randomUUID().toString() + '/' + System.nanoTime();
        System.setProperty("magic", magic);
        // the easiest way uses the non-standardized runtime name string
        String name = ManagementFactory.getRuntimeMXBean().getName();
        int ix = name.indexOf('@');
        if (ix >= 0) name = name.substring(0, ix);
        VirtualMachine vm;
        getVM:
        {
            try {
                vm = VirtualMachine.attach(name);
                if (magic.equals(vm.getSystemProperties().getProperty("magic")))
                    break getVM;
            } catch (Exception ex) {
            }
            //   if the easy way failed, try iterating over all local JVMs
            for (VirtualMachineDescriptor vd : VirtualMachine.list())
                try {
                    vm = VirtualMachine.attach(vd);
                    if (magic.equals(vm.getSystemProperties().getProperty("magic")))
                        break getVM;
                    vm.detach();
                } catch (Exception ex) {
                }
            //   could not find our own JVM or could not attach to it
            return;
        }
        System.out.println("attached to: " + vm.id() + '/' + vm.provider().type());
        vm.loadAgent(createJar().getAbsolutePath());
        synchronized (SelfAttacher.class) {
            while (BACK_LINK == null) SelfAttacher.class.wait();
        }
        System.out.println("Now I have hands on instrumentation: " + BACK_LINK);
        System.out.println(BACK_LINK.isModifiableClass(SelfAttacher.class));
        vm.detach();
    }

    // create a JAR file for the agent; since our class is already in class path
    // our jar consisting of a MANIFEST declaring our class as agent only

    private static final String AGENT_NAME = "energy.EnergyAgent";
    private static File createJar() throws IOException {
        File file = File.createTempFile("agent", ".jar");
        file.deleteOnExit();
        Charset cs = StandardCharsets.ISO_8859_1;
        try (FileOutputStream fos = new FileOutputStream(file);
             ZipOutputStream zos = new ZipOutputStream(fos)) {
            zos.putNextEntry(new ZipEntry("META-INF/MANIFEST.MF"));
            ByteBuffer bb = cs.encode("Agent-Class: " + SelfAttacher.class.getName());
            zos.write(bb.array(), bb.arrayOffset() + bb.position(), bb.remaining());
            zos.write(10);
            zos.closeEntry();
        }
        return file;
    }

    // invoked when the agent is loaded into the JVM, pass inst back to the caller
    public static void agentmain(String agentArgs, Instrumentation inst) {
        synchronized (SelfAttacher.class) {
            BACK_LINK = inst;
            SelfAttacher.class.notifyAll();
        }
    }
}


