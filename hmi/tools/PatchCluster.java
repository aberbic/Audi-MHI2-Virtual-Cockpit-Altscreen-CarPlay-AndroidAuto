import java.nio.file.*;
import java.util.*;
import java.util.zip.*;
import java.io.*;
import org.objectweb.asm.*;
import org.objectweb.asm.tree.*;

/** Modify exactly two classes from this car's converted stock ROM. Keep every
 * other method bytecode unchanged; never ship Qualcomm replacement classes. */
public final class PatchCluster implements Opcodes {
    static final String TARGET="de/audi/tghu/navi/app/cluster/CombiBAPListener";
    static final String SERVICE="de/audi/atip/interapp/combi/bap/navi/CombiBAPServiceNavi";
    static final String GATE="local/mu1438/ClusterGate";
    public static void main(String[] args) throws Exception {
        byte[] original;
        try(ZipFile zip=new ZipFile(args[0])) {
            original=zip.getInputStream(zip.getEntry(TARGET+".class")).readAllBytes();
        }
        ClassNode cn=new ClassNode();new ClassReader(original).accept(cn,0);
        cn.fields.add(new FieldNode(ACC_PUBLIC|ACC_STATIC|ACC_FINAL,"MU1438_CLUSTER_PATCH","I",null,1));
        int attach=0,zoom=0,position=0,scale=0,turn=0;
        for(MethodNode m:cn.methods) {
            if(m.name.equals("setCombiService") && m.desc.equals("(L"+SERVICE+";)V")) {
                InsnList in=new InsnList();in.add(new VarInsnNode(ALOAD,1));
                in.add(new MethodInsnNode(INVOKESTATIC,GATE,"attach",m.desc,false));
                m.instructions.insert(in);attach++;
            }
            if(m.name.equals("setMapScale") && m.desc.equals("(I)V")) {
                InsnList in=new InsnList();LabelNode stock=new LabelNode();
                in.add(new VarInsnNode(ILOAD,1));
                in.add(new MethodInsnNode(INVOKESTATIC,GATE,"zoom","(I)Z",false));
                in.add(new JumpInsnNode(IFEQ,stock));in.add(new InsnNode(RETURN));in.add(stock);
                m.instructions.insert(in);zoom++;
            }
            for(AbstractInsnNode ins:m.instructions.toArray()) {
                if(!(ins instanceof MethodInsnNode))continue;
                MethodInsnNode call=(MethodInsnNode)ins;
                if(call.getOpcode()!=INVOKEINTERFACE || !call.owner.equals(SERVICE))continue;
                String name=null;
                if(call.name.equals("updateCurrentPositionInfo") && call.desc.equals("(Ljava/lang/String;)V")){name="position";position++;}
                if(call.name.equals("updateTurnToInfo") && call.desc.equals("(Ljava/lang/String;Ljava/lang/String;)V")){name="turn";turn++;}
                if(call.name.equals("updateMapScale") && call.desc.equals("(IZIIZ)V")){name="scale";scale++;}
                if(name!=null)m.instructions.set(call,new MethodInsnNode(INVOKESTATIC,GATE,name,"(L"+SERVICE+";"+call.desc.substring(1),false));
            }
        }
        if(attach!=1 || zoom!=1 || position!=1 || scale!=1 || turn!=1)
            throw new IllegalStateException("unexpected MU1438 seams: "+attach+","+zoom+","+position+","+scale+","+turn);
        ClassWriter cw=new ClassWriter(ClassWriter.COMPUTE_MAXS);cn.accept(cw);
        Path out=Path.of(args[1],TARGET+".class");Files.createDirectories(out.getParent());Files.write(out,cw.toByteArray());
        System.out.println("Patched one stock class: attach, zoom, position, turn text, scale. Class version="+cn.version);
        final String dm="de/audi/tghu/fwhmi/DisplayManagerMIB2High";
        try(ZipFile zip=new ZipFile(args[0])) {
            original=zip.getInputStream(zip.getEntry(dm+".class")).readAllBytes();
        }
        cn=new ClassNode();new ClassReader(original).accept(cn,0);int switches=0;
        cn.fields.add(new FieldNode(ACC_PUBLIC|ACC_STATIC|ACC_FINAL,"MU1438_CLUSTER_PATCH","I",null,1));
        for(MethodNode m:cn.methods) {
            if(!m.name.equals("switchContext") || !m.desc.equals("(IILde/audi/atip/hmi/view/IDisplayListener;)V"))continue;
            for(AbstractInsnNode ins:m.instructions.toArray()) {
                if(!(ins instanceof MethodInsnNode))continue;
                MethodInsnNode call=(MethodInsnNode)ins;
                if(call.getOpcode()!=INVOKESPECIAL || !call.owner.equals("de/audi/tghu/fwhmi/DisplayManager") || !call.name.equals("switchContext"))continue;
                AbstractInsnNode start=call.getPrevious().getPrevious().getPrevious().getPrevious();
                if(!(start instanceof VarInsnNode) || start.getOpcode()!=ALOAD || ((VarInsnNode)start).var!=0)
                    throw new IllegalStateException("unexpected context call stack");
                LabelNode dispatch=new LabelNode();InsnList gate=new InsnList();gate.add(dispatch);
                gate.add(new VarInsnNode(ALOAD,0));gate.add(new VarInsnNode(ILOAD,1));gate.add(new VarInsnNode(ILOAD,2));
                gate.add(new MethodInsnNode(INVOKESTATIC,GATE,"context","(Lde/audi/atip/hmi/view/IDisplayManager;II)I",false));
                gate.add(new VarInsnNode(ISTORE,1));m.instructions.insertBefore(start,gate);
                // Custom context must bypass addKDKToContext's default -> ctx73.
                InsnList entry=new InsnList();entry.add(new VarInsnNode(ILOAD,1));entry.add(new IntInsnNode(SIPUSH,900));
                entry.add(new JumpInsnNode(IF_ICMPEQ,dispatch));m.instructions.insert(entry);switches++;
            }
        }
        if(switches!=1)throw new IllegalStateException("unexpected display seam count="+switches);
        // The converter promotes synthetic access$ methods to v49. This unit's
        // J9 accepts v48; ASM emits the equivalent legacy Synthetic attribute.
        // This class has no generics, enums, varargs, annotations or Java5 ops.
        if(cn.signature!=null)throw new IllegalStateException("unexpected generic DM class");
        for(MethodNode m:cn.methods)if(m.signature!=null || (m.access&(ACC_BRIDGE|ACC_VARARGS))!=0)
            throw new IllegalStateException("unexpected Java5 feature in "+m.name);
        cn.version=V1_4;
        cw=new ClassWriter(ClassWriter.COMPUTE_MAXS);cn.accept(cw);
        out=Path.of(args[1],dm+".class");Files.createDirectories(out.getParent());Files.write(out,cw.toByteArray());
        System.out.println("Patched stock display context seam; stock warnings/non-navigation contexts pass through.");
    }
}
