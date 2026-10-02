/** Separate JVM only: loads/verifies classes, never starts HMI services. */
public class VerifyOnUnit {
    public static void main(String[] args) throws Exception {
        String[] names={"de.audi.tghu.navi.app.cluster.CombiBAPListener",
                        "de.audi.tghu.fwhmi.DisplayManagerMIB2High"};
        for(int i=0;i<names.length;i++) {
            Class c=Class.forName(names[i],false,VerifyOnUnit.class.getClassLoader());
            c.getDeclaredField("MU1438_CLUSTER_PATCH");
            System.out.println("VERIFIED "+names[i]+" methods="+c.getDeclaredMethods().length);
        }
        Class c=Class.forName("local.mu1438.ClusterGate",false,VerifyOnUnit.class.getClassLoader());
        System.out.println("VERIFIED bridge methods="+c.getDeclaredMethods().length);
    }
}
