package local.mu1438;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.PrintStream;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import de.audi.atip.interapp.combi.bap.navi.CombiBAPServiceNavi;
import de.audi.atip.hmi.view.IDisplayManager;

/** MU1438-only bridge: native navigation cache stays intact, only its cluster
 * publication is gated. No changes to gauges, warning services or nav routing.
 * A live renderer heartbeat is required; disconnect/crash restores latest data. */
public final class ClusterGate implements Runnable {
    private static final Object LOCK = new Object();
    private static final File ACTIVE = new File(System.getProperty("mu1438.cluster.marker", "/tmp/mu1438-cluster-active"));
    private static final File DISABLE = new File(System.getProperty("mu1438.cluster.disable", "/tmp/mu1438-cluster-disable"));
    private static CombiBAPServiceNavi service;
    private static Thread worker;
    private static volatile boolean active;
    private static String activeLabel="CarPlay";
    private static volatile IDisplayManager display;
    private static volatile int requestedContext=72;
    private static int appliedContext=-1;
    private static String road, turn, sign;
    private static boolean haveRoad, haveTurn, haveScale;
    private static int scaleA, scaleC, scaleD, steps;
    private static boolean scaleB, scaleE;
    private static PrintStream log;

    private ClusterGate() {}
    private static boolean navigationContext(int context) {
        return context>=72 && context<=77;
    }
    /* Called after the stock KDK remap, only for terminal 1 (physical display 4).
     * Must never acquire LOCK: switchContext holds the display manager monitor. */
    public static int context(IDisplayManager dm,int context,int terminal) {
        if(terminal!=1)return context;
        display=dm;
        if(context!=900)requestedContext=context;
        if(active && live() && navigationContext(context))return 900;
        return context;
    }
    private static void reconcileDisplay() {
        IDisplayManager dm=display;
        if(dm==null)return;
        int current=dm.getCurrentContextID(1);
        int requested=requestedContext;
        if(active && live() && (navigationContext(current)||current==900) &&
           navigationContext(requested)) {
            if(current!=900){dm.switchContext(900,1,null);appliedContext=900;log("cluster context acquired");}
        } else if(!active && current==900) {
            dm.switchContext(requested==900?72:requested,1,null);
            appliedContext=-1;log("cluster context restored");
        }
    }
    private static void log(String s) {
        try {
            if (log == null) log = new PrintStream(new FileOutputStream("/tmp/mu1438-cluster-hmi.log", true));
            log.println(s); log.flush();
        } catch (Throwable ignored) {}
    }
    public static void attach(CombiBAPServiceNavi target) {
        synchronized (LOCK) {
            service = target;
            if (worker == null && target != null) {
                worker = new Thread(new ClusterGate(), "MU1438-ClusterGate");
                worker.setDaemon(true);
                try { worker.start(); log("bridge attached"); }
                catch (Throwable e) { worker = null; log("worker failed: " + e); }
            }
        }
    }
    private static boolean live() {
        long now = System.currentTimeMillis(), stamp = ACTIVE.lastModified();
        return !DISABLE.exists() && stamp > 0 && now >= stamp && now-stamp < 3000;
    }
    private static String projectionLabel() {
        FileInputStream in=null;
        try {
            byte[] data=new byte[96];in=new FileInputStream(ACTIVE);
            int n=in.read(data);if(n<=0)return null;
            String marker=new String(data,0,n);
            int end=marker.indexOf('\n');if(end<0)return null;
            if(marker.indexOf("\nAndroid Auto\n")>=0)return "Android Auto";
            // Existing CarPlay renderer writes only its PID and a newline.
            return end==marker.length()-1 ? "CarPlay" : null;
        } catch (Throwable ignored) { return null; }
        finally { if(in!=null)try{in.close();}catch(Throwable ignored){} }
    }
    private static void clear(CombiBAPServiceNavi s) {
        s.updateCurrentPositionInfo(activeLabel);
        s.updateTurnToInfo("", "");
        s.updateMapScale(0, false, 0, 0, false);
    }
    private static void restore(CombiBAPServiceNavi s) {
        if (haveRoad) s.updateCurrentPositionInfo(road);
        if (haveTurn) s.updateTurnToInfo(turn, sign);
        if (haveScale) s.updateMapScale(scaleA, scaleB, scaleC, scaleD, scaleE);
    }
    public static void position(CombiBAPServiceNavi s, String value) {
        synchronized (LOCK) {
            road=value; haveRoad=true;
            s.updateCurrentPositionInfo(active ? activeLabel : value);
        }
    }
    public static void turn(CombiBAPServiceNavi s, String a, String b) {
        synchronized (LOCK) {
            turn=a; sign=b; haveTurn=true;
            s.updateTurnToInfo(active ? "" : a, active ? "" : b);
        }
    }
    public static void scale(CombiBAPServiceNavi s, int a, boolean b, int c, int d, boolean e) {
        synchronized (LOCK) {
            scaleA=a;scaleB=b;scaleC=c;scaleD=d;scaleE=e;haveScale=true;
            if (active) s.updateMapScale(0,false,0,0,false);
            else s.updateMapScale(a,b,c,d,e);
        }
    }
    /** True consumes OEM zoom only while the renderer is alive. */
    public static boolean zoom(int amount) {
        synchronized (LOCK) {
            if (!active || !live()) return false;
            if (amount > 32) amount=32;
            if (amount < -32) amount=-32;
            steps+=amount;
            if (steps>32) steps=32;
            if (steps< -32) steps=-32;
            if (service!=null) service.updateMapScale(0,false,0,0,false);
            log("zoom steps=" + amount);
            LOCK.notifyAll();
            return true;
        }
    }
    public void run() {
        DatagramSocket socket=null;
        for (;;) {
            int send=0;
            try {
                synchronized (LOCK) {
                    boolean next=service!=null && live();
                    String label=next?projectionLabel():null;
                    boolean labelChanged=label!=null && !label.equals(activeLabel);
                    if(label!=null)activeLabel=label;
                    if (next!=active) {
                        active=next;steps=0;
                        if (active) clear(service); else if(service!=null) restore(service);
                        log(active ? "native labels suppressed" : "native labels restored");
                        if(!active)activeLabel="CarPlay";
                    } else if(active && labelChanged) {
                        service.updateCurrentPositionInfo(activeLabel);
                        log("projection label="+activeLabel);
                    }
                    if (active) { send=steps; steps=0; }
                }
                if (send!=0) {
                    if(socket==null)socket=new DatagramSocket();
                    byte[] data=new byte[]{77,90,48,49,(byte)(send>>24),(byte)(send>>16),(byte)(send>>8),(byte)send};
                    socket.send(new DatagramPacket(data,8,InetAddress.getByName("127.0.0.1"),19822));
                }
                reconcileDisplay();
                synchronized (LOCK) { LOCK.wait(200); }
            } catch (Throwable e) {
                log("bridge error: " + e);
                if(socket!=null)socket.close();socket=null;
                try { Thread.sleep(500); } catch (InterruptedException ignored) {}
            }
        }
    }
}
