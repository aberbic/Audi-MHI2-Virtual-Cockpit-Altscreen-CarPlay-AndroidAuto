import java.io.*;
import java.lang.reflect.*;
import java.net.*;
import java.nio.*;
import java.nio.file.*;
import java.util.*;
import local.mu1438.ClusterGate;
import de.audi.atip.interapp.combi.bap.navi.CombiBAPServiceNavi;
import de.audi.atip.hmi.view.IDisplayManager;

public class GateTest {
    static volatile String road;
    static volatile int scale=-1;
    static volatile int currentContext=72;
    static void check(boolean ok,String why){if(!ok)throw new AssertionError(why);}
    public static void main(String[] args)throws Exception {
        Path dir=Files.createTempDirectory("mu1438-gate-test-");
        File flag=dir.resolve("active").toFile();
        System.setProperty("mu1438.cluster.marker",flag.toString());
        System.setProperty("mu1438.cluster.disable",dir.resolve("disable").toString());
        CombiBAPServiceNavi service=(CombiBAPServiceNavi)java.lang.reflect.Proxy.newProxyInstance(
            GateTest.class.getClassLoader(),new Class[]{CombiBAPServiceNavi.class},(obj,m,a)->{
                if(m.getName().equals("updateCurrentPositionInfo"))road=(String)a[0];
                if(m.getName().equals("updateMapScale"))scale=(Integer)a[0];
                if(m.getReturnType()==boolean.class)return false;
                if(m.getReturnType()==int.class)return 0;
                return null;
            });
        ClusterGate.attach(service);
        IDisplayManager dm=(IDisplayManager)java.lang.reflect.Proxy.newProxyInstance(
            GateTest.class.getClassLoader(),new Class[]{IDisplayManager.class},(obj,m,a)->{
                if(m.getName().equals("getCurrentContextID"))return currentContext;
                if(m.getName().equals("switchContext"))currentContext=ClusterGate.context((IDisplayManager)obj,(Integer)a[0],(Integer)a[1]);
                if(m.getReturnType()==int.class)return 0;
                return null;
            });
        check(ClusterGate.context(dm,72,1)==72,"stock context passes through");
        ClusterGate.position(service,"stock road");ClusterGate.scale(service,123,false,1,2,false);
        check(road.equals("stock road")&&scale==123,"stock forwarding");
        check(!ClusterGate.zoom(1),"stock zoom must not be consumed");
        try(DatagramSocket receiver=new DatagramSocket(19822,InetAddress.getLoopbackAddress())) {
            receiver.setSoTimeout(2000);
            Files.write(flag.toPath(),new byte[]{1});Thread.sleep(450);
            check(road.equals("CarPlay")&&scale==0,"activation labels CarPlay and clears native scale");
            check(currentContext==900,"AltScreen takes over navigation context");
            check(ClusterGate.context(dm,75,1)==900,"route-end blank context stays on AltScreen");
            check(ClusterGate.context(dm,72,0)==72,"main screen untouched");
            dm.switchContext(35,1,null);Thread.sleep(250);
            check(currentContext==35,"non-navigation context not stolen");
            dm.switchContext(72,1,null);
            check(currentContext==900,"return to map retakes AltScreen");
            ClusterGate.position(service,"latest road");ClusterGate.scale(service,456,false,1,2,false);
            check(road.equals("CarPlay")&&scale==0,"active native publications cannot overwrite CarPlay label");
            Files.write(flag.toPath(),"123\nAndroid Auto\n".getBytes("US-ASCII"));Thread.sleep(350);
            check(road.equals("Android Auto"),"AA renderer selects Android Auto label");
            Files.write(flag.toPath(),new byte[0]);Thread.sleep(250);
            check(road.equals("Android Auto"),"partial heartbeat retains label without flicker");
            Files.write(flag.toPath(),"123\n".getBytes("US-ASCII"));Thread.sleep(350);
            check(road.equals("CarPlay"),"legacy CarPlay marker restores CarPlay label");
            check(ClusterGate.zoom(-3),"active zoom intercepted");
            byte[] b=new byte[8];DatagramPacket p=new DatagramPacket(b,b.length);receiver.receive(p);
            check(new String(b,0,4,"US-ASCII").equals("MZ01")&&ByteBuffer.wrap(b,4,4).getInt()==-3,"signed zoom packet");
            check(flag.setLastModified(System.currentTimeMillis()-5000),"expire test marker");
            Thread.sleep(450);
            check(road.equals("latest road")&&scale==456,"latest OEM state restored after stale heartbeat");
            check(!ClusterGate.zoom(1),"expired renderer restores stock zoom");
            check(currentContext==72,"disconnect restores last requested navigation context");
        }
        Class.forName("de.audi.tghu.navi.app.cluster.CombiBAPListener",false,GateTest.class.getClassLoader()).getDeclaredMethods();
        Class.forName("de.audi.tghu.fwhmi.DisplayManagerMIB2High",false,GateTest.class.getClassLoader()).getDeclaredMethods();
        System.out.println("PASS: forwarding, suppression, signed zoom, route-end takeover, warning passthrough, disconnect restoration, patched-class verification");
    }
}
