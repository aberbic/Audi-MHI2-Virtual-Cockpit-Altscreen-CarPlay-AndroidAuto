package local.mu1438;

import java.io.FileOutputStream;
import java.io.PrintStream;
import java.io.File;
import de.audi.app.terminalmode.keyevents.ITerminalModeDSIKeyEventsController;
import de.audi.app.terminalmode.keyevents.Key;
import de.audi.app.terminalmode.keyevents.KeyState;
import de.audi.app.terminalmode.keyevents.TouchEvent;
import org.dsi.ifc.keypanel.DSIKeyPanel;
import org.dsi.ifc.base.DSIListener;

/** MU1438 adaptation of binarybase's touchpad-to-DPAD concept.
 * Stock firmware classes/constructors and all non-CarPlay input remain intact.
 */
public final class TouchpadBridge {
    private static final Object LOCK=new Object();
    private static ITerminalModeDSIKeyEventsController target;
    private static boolean gesture,nativeDirection;
    private static long started,suppressMiddleUntil;
    private static Key direction;
    private static int generation;
    private static Thread pending;
    private static int logCount;
    private static final File TRACE=new File("/tmp/mu1438-touchpad-trace.enabled");
    private static final Object TRACE_LOCK=new Object();
    private static int traceCount;
    private TouchpadBridge() {}
    public static void trace(String event,int[] values) {
        try {
            if(!TRACE.exists())return;
            synchronized(TRACE_LOCK) {
                if(traceCount++>=512)return;
                StringBuffer line=new StringBuffer();line.append(System.currentTimeMillis()).append(' ').append(event);
                line.append(" carplay=").append(target!=null);
                for(int i=0;i<values.length;i++)line.append(' ').append(i).append('=').append(values[i]);
                PrintStream out=new PrintStream(new FileOutputStream("/tmp/mu1438-touchpad-input.log",true));
                out.println(line.toString());out.close();
            }
        } catch(Throwable ignored) {}
    }
    public static void traceTouches(TouchEvent[] touches) {
        try {
            if(!TRACE.exists())return;
            if(touches==null){trace("CarPlay.touchEvents",new int[]{-1});return;}
            trace("CarPlay.touchEvents",new int[]{touches.length});
            for(int i=0;i<touches.length && i<2;i++)if(touches[i]!=null){
                TouchEvent t=touches[i];
                trace("CarPlay.touch",new int[]{i,t.isTouchScreen()?1:0,t.getTouchState(),t.getStartX(),t.getStartY(),t.getCurrentX(),t.getCurrentY()});
            }
        } catch(Throwable ignored) {}
    }
    private static void log(String message) {
        try {
            if(logCount++>=256)return;
            PrintStream out=new PrintStream(new FileOutputStream("/tmp/mu1438-touchpad.log",true));
            out.println(message);out.close();
        } catch(Throwable ignored) {}
    }
    private static void cancel() {
        generation++;
        if(pending!=null){pending.interrupt();pending=null;}
    }
    private static void expire(long now) {
        if(gesture && (now<started || now-started>1500)){gesture=false;direction=null;cancel();}
    }
    public static void bind(ITerminalModeDSIKeyEventsController controller) {
        synchronized(LOCK) {
            cancel();gesture=false;nativeDirection=false;direction=null;suppressMiddleUntil=0;
            target=controller!=null && controller.getClass().getName().equals(
                "de.audi.app.terminalmode.dsi.carplay.CarplayDSILifecycleController$TerminalModeDSIKeyEventsController")?controller:null;
            log(target==null?"CarPlay touchpad inactive; stock input passthrough":"CarPlay touchpad bound");
        }
    }
    public static void notifications(DSIKeyPanel panel,int[] attrs,DSIListener listener) {
        for(int i=0;i<attrs.length;i++)if(attrs[i]==8){panel.setNotification(attrs,listener);return;}
        int[] extended=new int[attrs.length+1];System.arraycopy(attrs,0,extended,0,attrs.length);extended[attrs.length]=8;
        panel.setNotification(extended,listener);
    }
    public static void encoder(int keyboard,int axis,int delta) {
        synchronized(LOCK) {
            expire(System.currentTimeMillis());
            if(target==null || keyboard!=9 || !gesture || delta==0 || direction!=null)return;
            if(axis==76)direction=delta<0?Key.JS_WEST:Key.JS_EAST;
            else if(axis==77)direction=delta<0?Key.JS_NORTH:Key.JS_SOUTH;
            if(direction!=null)cancel();
        }
    }
    private static void emit(ITerminalModeDSIKeyEventsController controller,Key key,int ticks) {
        try {
            for(int i=0;i<ticks;i++) {
                synchronized(LOCK){if(target!=controller)return;}
                controller.updateKey(key,KeyState.PRESSED);
                // Stock MU1438 releases a latched joystick direction on MIDDLE.
                controller.updateKey(key==Key.DDS_SELECT?key:Key.JS_MIDDLE,
                                     key==Key.DDS_SELECT?KeyState.RELEASED:KeyState.PRESSED);
            }
        } catch(Throwable failure) {log("touchpad dispatch failed: "+failure.getClass().getName());}
    }
    private static void deferTap(final ITerminalModeDSIKeyEventsController controller) {
        cancel();final int token=generation;
        try {
            pending=new Thread(new Runnable(){public void run(){
                try {
                    Thread.sleep(250);
                    synchronized(LOCK){
                        if(token!=generation || target!=controller || gesture || System.currentTimeMillis()<suppressMiddleUntil)return;
                        pending=null;
                    }
                    emit(controller,Key.DDS_SELECT,1);log("touchpad select");
                } catch(InterruptedException ignored) {}
            }},"MU1438-TouchpadTap");
            pending.setDaemon(true);pending.start();
        } catch(Throwable failure){pending=null;log("touchpad tap timer unavailable");}
    }
    public static void gesture(int keyboard,int type) {
        ITerminalModeDSIKeyEventsController controller=null;Key key=null;int ticks=0;
        synchronized(LOCK) {
            if(target==null || keyboard!=9)return;
            long now=System.currentTimeMillis();expire(now);
            if(type==4){cancel();gesture=true;direction=null;started=now;return;}
            if(type!=3 || !gesture)return;
            controller=target;key=direction;long elapsed=now-started;
            gesture=false;direction=null;cancel();
            if(key!=null){ticks=elapsed>=0 && elapsed<150?3:1;suppressMiddleUntil=now+250;}
            else deferTap(controller);
        }
        if(key!=null){emit(controller,key,ticks);log("touchpad swipe ticks="+ticks);}
    }
    public static void rotary(ITerminalModeDSIKeyEventsController controller,int amount) {
        synchronized(LOCK){
            if(controller==target && target!=null){expire(System.currentTimeMillis());cancel();if(gesture)return;}
        }
        controller.updateRotary(amount);
    }
    public static void key(ITerminalModeDSIKeyEventsController controller,Key key,KeyState state) {
        synchronized(LOCK) {
            if(controller==target && target!=null){
                long now=System.currentTimeMillis();expire(now);
                if(key==Key.JS_MIDDLE){
                    if(nativeDirection){nativeDirection=false;cancel();}
                    else {
                        if(gesture || now<suppressMiddleUntil)return;
                        deferTap(controller);return;
                    }
                } else {
                    cancel();
                    if(key==Key.JS_NORTH || key==Key.JS_SOUTH || key==Key.JS_EAST || key==Key.JS_WEST){
                        if(gesture || now<suppressMiddleUntil)return;
                        nativeDirection=true;
                    }
                }
            }
        }
        controller.updateKey(key,state);
    }
}
