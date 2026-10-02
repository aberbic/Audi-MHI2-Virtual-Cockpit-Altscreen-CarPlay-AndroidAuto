import java.nio.file.*;
import java.util.*;
import java.util.zip.*;
import local.mu1438.NativeNavObserver;
import de.audi.atip.interapp.combi.bap.navi.data.CombiBAPNaviManeuverDescriptor;

public final class ObserverTest {
    public static void main(String[] args) throws Exception {
        Path dir=Files.createTempDirectory("mu1438-observer-test-");
        Path enable=dir.resolve("enable"), output=dir.resolve("events.log");
        System.setProperty("mu1438.nav.observe.enable",enable.toString());
        System.setProperty("mu1438.nav.observe.output",output.toString());
        NativeNavObserver.record("disabled",new Object[]{"private destination"});
        if(Files.exists(output))throw new AssertionError("Not opt-in");
        Files.createFile(enable);
        CombiBAPNaviManeuverDescriptor d=new CombiBAPNaviManeuverDescriptor(7,8,9,new byte[]{1,2});
        Object[] values={"private destination",Integer.valueOf(123),Boolean.TRUE,Long.valueOf(456),new CombiBAPNaviManeuverDescriptor[]{d}};
        NativeNavObserver.record("sample",values);
        if(d.mainElement!=7 || d.direction!=8 || d.zLevelGuidance!=9 || !Arrays.equals(d.sideStreets,new byte[]{1,2}))throw new AssertionError("Input mutated");
        for(int i=0;i<4100;i++)NativeNavObserver.record("bounded",new Object[0]);
        String result=Files.readString(output);
        if(result.contains("private destination") || !result.contains("text[length=19]") || !result.contains("{7,8,9}"))throw new AssertionError("Redaction/shape failed");
        if(Files.readAllLines(output).size()!=4001)throw new AssertionError("Event cap failed");
        try(ZipFile base=new ZipFile(args[0]); ZipFile patched=new ZipFile(args[1])) {
            for(ZipEntry e:Collections.list(base.entries())) {
                if(!Arrays.equals(base.getInputStream(e).readAllBytes(),patched.getInputStream(patched.getEntry(e.getName())).readAllBytes()))throw new AssertionError("Original entry changed: "+e.getName());
            }
            if(patched.size()!=base.size()+2)throw new AssertionError("Unexpected entries");
        }
        System.out.println("PASS: disabled by default, text redacted, descriptors untouched, 4000-event cap, existing projection classes byte-identical.");
    }
}
