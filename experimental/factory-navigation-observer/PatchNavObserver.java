import java.nio.file.*;
import java.util.*;
import java.util.zip.*;
import org.objectweb.asm.*;
import org.objectweb.asm.tree.*;

/** Add a guarded observer prefix; leave the existing projection classes byte-exact. */
public final class PatchNavObserver implements Opcodes {
    static final String TARGET="de/audi/app/combi/bap/app/navi/AppConnectorNavi";
    static final String OBS="local/mu1438/NativeNavObserver";
    static final Set<String> METHODS=new HashSet<>(Arrays.asList(
        "updateRGStatus", "updateActiveRGType", "updateManeuverState",
        "updateManeuverDescriptor", "updateManeuverDescriptorAndExitView",
        "updateDistanceToNextManeuver", "updateCurrentPositionInfo", "updateTurnToInfo",
        "updateDistanceToDestination", "updateTimeToDestination", "updateLaneGuidance",
        "updateExitView", "updateMapVisibility"));
    public static void main(String[] args) throws Exception {
        ClassNode c=new ClassNode();
        try(ZipFile stock=new ZipFile(args[0])) {
            new ClassReader(stock.getInputStream(stock.getEntry(TARGET+".class"))).accept(c,0);
        }
        if(c.version>V1_4) throw new IllegalStateException("Unexpected class version "+c.version);
        Set<String> found=new HashSet<>();
        for(MethodNode m:c.methods) {
            if(!METHODS.contains(m.name)) continue;
            if(!found.add(m.name) || (m.access&ACC_STATIC)!=0) throw new IllegalStateException("Unexpected method "+m.name);
            LabelNode start=new LabelNode(), end=new LabelNode(), handler=new LabelNode(), resume=new LabelNode();
            InsnList p=new InsnList();p.add(start);p.add(new LdcInsnNode(m.name));
            Type[] types=Type.getArgumentTypes(m.desc);
            p.add(new IntInsnNode(BIPUSH,types.length));p.add(new TypeInsnNode(ANEWARRAY,"java/lang/Object"));
            int local=1;
            for(int i=0;i<types.length;i++) {
                Type t=types[i];p.add(new InsnNode(DUP));p.add(new IntInsnNode(BIPUSH,i));
                if(t.getSort()==Type.INT || t.getSort()==Type.BOOLEAN || t.getSort()==Type.LONG) {
                    String box=t.getSort()==Type.LONG?"java/lang/Long":t.getSort()==Type.BOOLEAN?"java/lang/Boolean":"java/lang/Integer";
                    p.add(new TypeInsnNode(NEW,box));p.add(new InsnNode(DUP));
                    p.add(new VarInsnNode(t.getOpcode(ILOAD),local));
                    p.add(new MethodInsnNode(INVOKESPECIAL,box,"<init>","("+t.getDescriptor()+")V",false));
                } else if(t.getSort()==Type.OBJECT || t.getSort()==Type.ARRAY) p.add(new VarInsnNode(ALOAD,local));
                else throw new IllegalStateException("Unexpected argument "+t);
                p.add(new InsnNode(AASTORE));local+=t.getSize();
            }
            p.add(new MethodInsnNode(INVOKESTATIC,OBS,"record","(Ljava/lang/String;[Ljava/lang/Object;)V",false));
            p.add(end);p.add(new JumpInsnNode(GOTO,resume));p.add(handler);p.add(new InsnNode(POP));p.add(resume);
            m.instructions.insert(p);
            m.tryCatchBlocks.add(0,new TryCatchBlockNode(start,end,handler,"java/lang/Throwable"));
        }
        if(!found.equals(METHODS))throw new IllegalStateException("Missing methods "+found);
        ClassWriter cw=new ClassWriter(ClassWriter.COMPUTE_MAXS);c.accept(cw);
        try(ZipFile base=new ZipFile(args[1]); ZipOutputStream out=new ZipOutputStream(Files.newOutputStream(Path.of(args[3])))) {
            for(ZipEntry e:Collections.list(base.entries())) {
                if(e.getName().equals(TARGET+".class") || e.getName().equals(OBS+".class"))throw new IllegalStateException("Already patched");
                out.putNextEntry(new ZipEntry(e.getName()));base.getInputStream(e).transferTo(out);out.closeEntry();
            }
            out.putNextEntry(new ZipEntry(TARGET+".class"));out.write(cw.toByteArray());out.closeEntry();
            out.putNextEntry(new ZipEntry(OBS+".class"));out.write(Files.readAllBytes(Path.of(args[2])));out.closeEntry();
        }
        System.out.println("Added observer at "+found.size()+" factory navigation entry points; original projection entries copied unchanged.");
    }
}
