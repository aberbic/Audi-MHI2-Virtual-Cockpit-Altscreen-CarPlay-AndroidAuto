package local.mu1438;

import java.io.File;
import java.io.FileOutputStream;
import java.io.PrintStream;
import de.audi.atip.interapp.combi.bap.navi.data.CombiBAPNaviManeuverDescriptor;

/** Opt-in, bounded observation only. Never publishes cluster state. */
public final class NativeNavObserver {
    private static final File ENABLE = new File(System.getProperty("mu1438.nav.observe.enable", "/tmp/mu1438-nav-observe.enabled"));
    private static final String OUTPUT = System.getProperty("mu1438.nav.observe.output", "/tmp/mu1438-nav-observe.log");
    private static PrintStream out;
    private static int count;
    private static boolean failed;
    private NativeNavObserver() {}

    public static synchronized void record(String method, Object[] args) {
        try {
            if (failed || count >= 4000 || !ENABLE.exists()) return;
            if (out == null) out = new PrintStream(new FileOutputStream(OUTPUT, false));
            StringBuffer line = new StringBuffer();
            line.append(System.currentTimeMillis()).append(' ').append(method);
            for (int i=0; i<args.length; i++) {
                Object value=args[i];
                line.append(' ').append(i).append('=');
                if(value==null) line.append("null");
                else if(value instanceof String) line.append("text[length=").append(((String)value).length()).append(']');
                else if(value instanceof Number || value instanceof Boolean) line.append(value);
                else if(value instanceof CombiBAPNaviManeuverDescriptor[]) {
                    CombiBAPNaviManeuverDescriptor[] a=(CombiBAPNaviManeuverDescriptor[])value;
                    line.append("maneuvers[count=").append(a.length).append(']');
                    for(int j=0;j<a.length && j<3;j++) {
                        if(a[j]==null) {line.append(" null");continue;}
                        line.append(" {").append(a[j].mainElement).append(',').append(a[j].direction).append(',').append(a[j].zLevelGuidance).append('}');
                    }
                } else if(value instanceof Object[]) line.append("array[count=").append(((Object[])value).length).append(']');
                else line.append("object");
            }
            out.println(line.toString());count++;
            if(count==4000) out.println("LIMIT: 4000 events; observation stopped");
            out.flush();
            if(out.checkError()) failed=true;
        } catch(Throwable ignored) { failed=true; }
    }
}
