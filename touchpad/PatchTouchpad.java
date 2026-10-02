import java.nio.file.*;
import java.util.*;
import java.util.zip.*;
import org.objectweb.asm.*;
import org.objectweb.asm.tree.*;

/** Patch exact MU1438 methods instead of installing MU0910 replacement classes. */
public final class PatchTouchpad implements Opcodes {
    static final String PANEL="de/audi/app/terminalmode/dsi/keypanel/TMDSIKeyPanelControllerImpl";
    static final String VBL="de/audi/app/terminalmode/keyevents/TMVirtualButtonListener";
    static final String CTRL="de/audi/app/terminalmode/keyevents/ITerminalModeDSIKeyEventsController";
    static final String HELPER="local/mu1438/TouchpadBridge";
    static final String CARPLAY="de/audi/app/terminalmode/dsi/carplay/CarplayDSILifecycleController$TerminalModeDSIKeyEventsController";
    static void traceInts(MethodNode m,String label) {
        Type[] args=Type.getArgumentTypes(m.desc);InsnList p=new InsnList();
        p.add(new LdcInsnNode(label));p.add(new IntInsnNode(BIPUSH,args.length));p.add(new IntInsnNode(NEWARRAY,T_INT));
        for(int i=0;i<args.length;i++){
            if(args[i].getSort()!=Type.INT && args[i].getSort()!=Type.BOOLEAN)throw new IllegalStateException("Non-scalar trace argument");
            p.add(new InsnNode(DUP));p.add(new IntInsnNode(BIPUSH,i));p.add(new VarInsnNode(ILOAD,i+1));p.add(new InsnNode(IASTORE));
        }
        p.add(new MethodInsnNode(INVOKESTATIC,HELPER,"trace","(Ljava/lang/String;[I)V",false));m.instructions.insert(p);
    }
    static byte[] patch(byte[] original,String name) {
        ClassNode c=new ClassNode();new ClassReader(original).accept(c,0);
        int encoder=0,gesture=0,notifications=0,bind=0,keys=0,rotary=0,touch=0;
        for(MethodNode m:c.methods) {
            if(name.equals(PANEL) && (m.name.equals("updateEncoder2") || m.name.equals("updateGesture2"))){
                boolean enc=m.name.equals("updateEncoder2");
                if(!m.desc.equals(enc?"(IIIII)V":"(IIIZIIIIII)V"))throw new IllegalStateException(m.desc);
                LabelNode skip=new LabelNode();InsnList p=new InsnList();
                p.add(new VarInsnNode(ALOAD,0));p.add(new VarInsnNode(ILOAD,enc?5:10));
                p.add(new MethodInsnNode(INVOKEVIRTUAL,PANEL,"isValid","(I)Z",false));p.add(new JumpInsnNode(IFEQ,skip));
                p.add(new VarInsnNode(ILOAD,1));p.add(new VarInsnNode(ILOAD,2));if(enc)p.add(new VarInsnNode(ILOAD,3));
                p.add(new MethodInsnNode(INVOKESTATIC,HELPER,enc?"encoder":"gesture",enc?"(III)V":"(II)V",false));p.add(skip);
                m.instructions.insert(p);if(enc)encoder++;else gesture++;
            }
            if(name.equals(VBL) && m.name.equals("setDSIKeyEventController") && m.desc.equals("(L"+CTRL+";)V")){
                for(AbstractInsnNode ins:m.instructions.toArray())if(ins.getOpcode()==RETURN){
                    InsnList p=new InsnList();p.add(new VarInsnNode(ALOAD,1));p.add(new MethodInsnNode(INVOKESTATIC,HELPER,"bind",m.desc,false));m.instructions.insertBefore(ins,p);bind++;
                }
            }
            for(AbstractInsnNode ins:m.instructions.toArray())if(ins instanceof MethodInsnNode){
                MethodInsnNode call=(MethodInsnNode)ins;
                if(name.equals(PANEL) && m.name.equals("addDSIService") && call.getOpcode()==INVOKEINTERFACE && call.name.equals("setNotification") && call.desc.equals("([ILorg/dsi/ifc/base/DSIListener;)V")){
                    m.instructions.set(call,new MethodInsnNode(INVOKESTATIC,HELPER,"notifications","(Lorg/dsi/ifc/keypanel/DSIKeyPanel;[ILorg/dsi/ifc/base/DSIListener;)V",false));notifications++;
                }
                if(name.equals(VBL) && call.getOpcode()==INVOKEINTERFACE && call.owner.equals(CTRL)){
                    if(call.name.equals("updateKey")){m.instructions.set(call,new MethodInsnNode(INVOKESTATIC,HELPER,"key","(L"+CTRL+";"+call.desc.substring(1),false));keys++;}
                    if(call.name.equals("updateRotary")){m.instructions.set(call,new MethodInsnNode(INVOKESTATIC,HELPER,"rotary","(L"+CTRL+";I)V",false));rotary++;}
                }
            }
            if(name.equals(PANEL) && (m.name.equals("updateEncoder2") || m.name.equals("updateGesture2") || m.name.equals("updateKey2")))traceInts(m,"DSI."+m.name);
            if(name.equals(VBL) && (m.name.startsWith("stick") || m.name.startsWith("touchPad") || m.name.equals("decrement") || m.name.equals("increment")))traceInts(m,"VBL."+m.name);
            if(name.equals(CARPLAY) && m.name.equals("updateTouchEvent")){traceInts(m,"CarPlay.touchEvent");touch++;}
            if(name.equals(CARPLAY) && m.name.equals("updateTouchEvents")){
                if(!m.desc.equals("([Lde/audi/app/terminalmode/keyevents/TouchEvent;)V"))throw new IllegalStateException(m.desc);
                InsnList p=new InsnList();p.add(new VarInsnNode(ALOAD,1));p.add(new MethodInsnNode(INVOKESTATIC,HELPER,"traceTouches",m.desc,false));m.instructions.insert(p);touch++;
            }
        }
        if(name.equals(PANEL) && (encoder!=1 || gesture!=1 || notifications!=1))throw new IllegalStateException("Panel seams changed");
        if(name.equals(VBL) && (bind!=1 || keys!=7 || rotary!=2))throw new IllegalStateException("VBL seams: "+bind+","+keys+","+rotary);
        if(name.equals(CARPLAY) && touch!=2)throw new IllegalStateException("CarPlay touch seams changed");
        if(c.signature!=null)throw new IllegalStateException("Unexpected generic class");
        c.version=V1_4;ClassWriter cw=new ClassWriter(ClassWriter.COMPUTE_MAXS);c.accept(cw);
        System.out.println("Patched MU1438 "+name+"; input calls="+keys+" rotary="+rotary);
        return cw.toByteArray();
    }
    public static void main(String[] args) throws Exception {
        Map<String,byte[]> additions=new LinkedHashMap<>();
        try(ZipFile stock=new ZipFile(args[0])){
            for(String name:Arrays.asList(PANEL,VBL,CARPLAY))additions.put(name+".class",patch(stock.getInputStream(stock.getEntry(name+".class")).readAllBytes(),name));
        }
        Path classes=Path.of(args[2]);
        try(var paths=Files.walk(classes)){for(Path p:paths.filter(Files::isRegularFile).toList())additions.put(classes.relativize(p).toString(),Files.readAllBytes(p));}
        try(ZipFile base=new ZipFile(args[1]);ZipOutputStream out=new ZipOutputStream(Files.newOutputStream(Path.of(args[3])))){
            for(ZipEntry e:Collections.list(base.entries())){
                if(additions.containsKey(e.getName()))throw new IllegalStateException("Refusing to overwrite prior patch: "+e.getName());
                out.putNextEntry(new ZipEntry(e.getName()));base.getInputStream(e).transferTo(out);out.closeEntry();
            }
            for(var e:additions.entrySet()){out.putNextEntry(new ZipEntry(e.getKey()));out.write(e.getValue());out.closeEntry();}
        }
    }
}
