import java.lang.reflect.*;
import java.util.*;
import java.util.zip.*;
import local.mu1438.TouchpadBridge;
import de.audi.app.terminalmode.keyevents.*;

public final class TouchpadTest {
    static class Fake implements ITerminalModeDSIKeyEventsController {
        final List<String> events=Collections.synchronizedList(new ArrayList<>());
        public void updateKey(Key k,KeyState s){events.add((k==Key.DDS_SELECT?"select":k==Key.JS_MIDDLE?"middle":k==Key.JS_NORTH?"north":k==Key.JS_EAST?"east":"other")+":"+(s==KeyState.PRESSED?"down":"up"));}
        public void updateRotary(int n){events.add("rotary:"+n);}
        public void updateTouchEvent(int a,int b,int c,int d,int e,int f,int g){}
        public void updateTouchEvents(TouchEvent[] e){}
        public void updateCharacterEvent(String[] c,int[] n){}
    }
    static void set(String name,Object v)throws Exception{Field f=TouchpadBridge.class.getDeclaredField(name);f.setAccessible(true);f.set(null,v);}
    static void active(Fake f)throws Exception{TouchpadBridge.bind(null);set("target",f);f.events.clear();}
    static void equal(Object a,Object b){if(!a.equals(b))throw new AssertionError(a+" != "+b);}
    public static void main(String[] args)throws Exception{
        Fake f=new Fake();TouchpadBridge.bind(f);
        TouchpadBridge.gesture(9,4);TouchpadBridge.encoder(9,77,-1);TouchpadBridge.gesture(9,3);
        TouchpadBridge.key(f,Key.JS_MIDDLE,KeyState.PRESSED);TouchpadBridge.rotary(f,3);
        equal(f.events,Arrays.asList("middle:down","rotary:3")); // non-CarPlay untouched
        active(f);TouchpadBridge.key(f,Key.JS_MIDDLE,KeyState.PRESSED);Thread.sleep(350);
        equal(f.events,Arrays.asList("select:down","select:up"));
        active(f);TouchpadBridge.key(f,Key.JS_MIDDLE,KeyState.PRESSED);
        TouchpadBridge.gesture(9,4);TouchpadBridge.encoder(9,77,-1);TouchpadBridge.rotary(f,2);TouchpadBridge.gesture(9,3);Thread.sleep(350);
        equal(f.events,Arrays.asList("north:down","middle:down","north:down","middle:down","north:down","middle:down"));
        active(f);TouchpadBridge.gesture(9,4);set("started",System.currentTimeMillis()-200);
        TouchpadBridge.encoder(9,76,1);TouchpadBridge.gesture(9,3);
        equal(f.events,Arrays.asList("east:down","middle:down"));
        active(f);TouchpadBridge.key(f,Key.JS_NORTH,KeyState.PRESSED);TouchpadBridge.key(f,Key.JS_MIDDLE,KeyState.PRESSED);Thread.sleep(350);
        equal(f.events,Arrays.asList("north:down","middle:down")); // physical joystick retained
        active(f);TouchpadBridge.key(f,Key.JS_MIDDLE,KeyState.PRESSED);TouchpadBridge.rotary(f,-4);Thread.sleep(350);
        equal(f.events,Arrays.asList("rotary:-4"));
        active(f);TouchpadBridge.key(f,Key.JS_MIDDLE,KeyState.PRESSED);TouchpadBridge.bind(null);Thread.sleep(350);equal(f.events,Collections.emptyList());
        active(f);TouchpadBridge.gesture(9,4);set("started",System.currentTimeMillis()-2000);TouchpadBridge.rotary(f,5);equal(f.events,Arrays.asList("rotary:5"));
        active(f);TouchpadBridge.gesture(8,4);TouchpadBridge.encoder(8,77,1);TouchpadBridge.gesture(8,3);equal(f.events,Collections.emptyList());
        TouchpadBridge.bind(null);
        try(ZipFile before=new ZipFile(args[0]);ZipFile after=new ZipFile(args[1])){
            for(ZipEntry e:Collections.list(before.entries())){
                if(!Arrays.equals(before.getInputStream(e).readAllBytes(),after.getInputStream(after.getEntry(e.getName())).readAllBytes()))throw new AssertionError("Changed existing cluster patch "+e.getName());
            }
            if(after.size()!=before.size()+5)throw new AssertionError("Unexpected class additions "+after.size());
        }
        System.out.println("PASS: CarPlay-only scope, tap, slow/fast swipe, rotary cancellation, physical joystick, backend switch, gesture timeout, existing cluster classes unchanged");
    }
}
