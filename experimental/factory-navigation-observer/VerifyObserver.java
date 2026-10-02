/** Separate JVM, no service instances and no static initialization. */
public final class VerifyObserver {
    public static void main(String[] args) throws Exception {
        String[] names={"de.audi.app.combi.bap.app.navi.AppConnectorNavi", "local.mu1438.NativeNavObserver"};
        for(int i=0;i<names.length;i++) {
            Class c=Class.forName(names[i],false,VerifyObserver.class.getClassLoader());
            System.out.println("VERIFIED "+names[i]+" methods="+c.getDeclaredMethods().length);
        }
    }
}
