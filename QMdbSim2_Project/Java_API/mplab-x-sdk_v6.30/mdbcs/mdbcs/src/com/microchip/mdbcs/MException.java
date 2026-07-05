package com.microchip.mdbcs;

/**
 * To keep Debugger simple, all functions throw an MException if any problem
 * happens.
 *
 */
public class MException extends Exception {

    public MException(String string) {
        super(string);
    }

    public MException(String string, Throwable thrwbl) {
        super(string, thrwbl);
    }

    public MException(Throwable thrwbl) {
        super(thrwbl);
    }

}
