package energy.cft;

import java.io.IOException;
import java.lang.instrument.ClassFileTransformer;
import java.security.ProtectionDomain;

public class EnergyLabeledTransformer implements ClassFileTransformer {
    private final String targetClassName;
    private final ClassLoader targetClassLoader;

    public EnergyLabeledTransformer(String targetClassName, ClassLoader targetClassLoader) {
        this.targetClassName = targetClassName;
        this.targetClassLoader = targetClassLoader;
    }

    @Override
    public byte[] transform(ClassLoader loader, String className, Class<?> classBeingRedefined, ProtectionDomain protectionDomain, byte[] classfileBuffer) {
        byte[] byteCode = classfileBuffer;
        String finalTargetClassName = targetClassName.replaceAll("\\.", "/");

        if (!className.equals(finalTargetClassName)) {
            return byteCode;
        }

//        if (className.equals(finalTargetClassName) && loader.equals(targetClassLoader)) {
//            System.out.println("[EngeryAgent] Transforming class Lock");
//            try {
//                ClassPool cp = ClassPool.getDefault();
//                CtClass cc = cp.get(targetClassName);
//                CtMethod m = cc.getDeclaredMethod(WITHDRAW_MONEY_METHOD);
//                m.addLocalVariable("startTime", CtClass.longType);
//                m.insertBefore("startTime = System.currentTimeMillis();");
//
//                StringBuilder endBlock = new StringBuilder();
//
//                m.addLocalVariable("endTime", CtClass.longType);
//                m.addLocalVariable("opTime", CtClass.longType);
//                endBlock.append("endTime = System.currentTimeMillis();");
//                endBlock.append("opTime = (endTime-startTime)/1000;");
//
//                endBlock.append("LOGGER.info(\"[Application] Withdrawal operation completed in:" +
//                                "\" + opTime + \" seconds!\");");
//
//                m.insertAfter(endBlock.toString());
//
//                byteCode = cc.toBytecode();
//                cc.detach();
//            } catch (NotFoundException | CannotCompileException | IOException e) {
//                throw new RuntimeException(e);
//            }
//        }
        return byteCode;
    }
}
