package com.microchip.mdbcs;


/**
 * QMdbCS -- thin façade over MDBCS Debugger for use by QSpice C-Block
 * components via JNI.  Aggregates related calls to minimize JNI boundary
 * crossings.
 *
 * C++ holds a QMdbCS instance and calls its methods directly.  Pin objects
 * returned by getPin() are cached as JNI global refs on the C++ side and
 * passed back in on each getPinState() call -- no per-call name lookup.
 */
public class QMdbCS {

    private final Debugger dbg;

    // ── Version ───────────────────────────────────────────────────────────

    public static String getVersion() { return "0.13.0"; }

    // ── Construction / lifecycle ──────────────────────────────────────────

    public QMdbCS(String device, String tool, boolean asDebugger)
            throws MException {
        dbg = new Debugger(device, tool, asDebugger);
    }

    public void loadFile(String path)   throws MException { dbg.loadFile(path); }
    public void connect()               throws MException { dbg.connect(); }
    public void program()               throws MException { dbg.program(); }
    public void reset()                 throws MException { dbg.reset(); }
    public void disconnect()                             { dbg.disconnect(); }
    public void destroy()                                { dbg.destroy(); }
    public long stepInstr() throws MException { return dbg.stepInstr(); }
    public void setVDD(float vdd)       throws MException { dbg.setVDD(vdd); }
    public java.util.Properties getToolProperties()      { return dbg.getToolProperties(); }

    // ── Pin access ────────────────────────────────────────────────────────

    /**
     * Returns a Pin object for the named pin.  The C++ side promotes the
     * returned reference to a JNI global ref and caches it for the session.
     */
    public Pin getPin(String name) throws MException { return dbg.getPin(name); }

    // ── Aggregated pin state -- one JNI call per pin per step ─────────────
    //
    // Returns a packed long:
    //
    //   Bits 63..32  voltage as IEEE 754 float bits (Float.floatToRawIntBits)
    //   Bit  1       1 = digital output, 0 = input / hi-Z
    //   Bit  0       1 = analog, 0 = digital
    //
    // Voltage precision: the underlying Debugger returns a double, but only
    // 32 bits are available in the packed long, so it is cast to float before
    // packing.  This gives ~7 significant decimal digits (~0.0000002 V at
    // 3.3 V).  The precision loss is intentional and considered acceptable
    // for pin voltage monitoring at simulator step rates.
    //
    // ADState is queried once via isAnalog(); isDigitalOutput() is only
    // called when the pin is known to be digital, so getADState() is called
    // exactly once per pin per step.
    // The Pin object is a cached global ref passed in from C++ -- no lookup.

    public long getPinState(Pin pin) {
        boolean analog  = pin.isAnalog();
        boolean output  = !analog && pin.isDigitalOutput();
        float   voltage = (float) pin.getVoltage();   // double→float: ~7 sig. digits, acceptable
        return ((long) Float.floatToRawIntBits(voltage) << 32)
             | (output ? 2L : 0L)
             | (analog ? 1L : 0L);
    }

    // ── External stimulus ─────────────────────────────────────────────────

    public void pinExternalSetVoltage(Pin pin, double v) {
        pin.externalSetVoltage(v);
    }
}