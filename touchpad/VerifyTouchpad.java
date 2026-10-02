/** Load/link only in a separate JVM; does not create HMI services. */
public final class VerifyTouchpad {
    public static void main(String[] args)throws Exception{
        String[] names={"local.mu1438.TouchpadBridge",
          "de.audi.app.terminalmode.dsi.keypanel.TMDSIKeyPanelControllerImpl",
          "de.audi.app.terminalmode.keyevents.TMVirtualButtonListener",
          "de.audi.app.terminalmode.dsi.carplay.CarplayDSILifecycleController$TerminalModeDSIKeyEventsController"};
        for(int i=0;i<names.length;i++){
            Class c=Class.forName(names[i],false,VerifyTouchpad.class.getClassLoader());
            System.out.println("VERIFIED "+names[i]+" methods="+c.getDeclaredMethods().length);
        }
    }
}
