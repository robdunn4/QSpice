/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package com.microchip.mdbcs;

import com.microchip.crownking.Pair;
import org.openide.util.Exceptions;
import java.util.*;
import java.io.*;

/**
 * Class providing helper methods. The most useful ones are the ones
 * dealing with unsigned ints which Java does not support natively.
 */public class Helper {
    private static int maxErrors = 10;

    /**
     * Mimic Windows sleep function
     * @param msecs
     */
    public static void sleep(int msecs) {
        try {
            Thread.sleep(msecs);
        } catch (InterruptedException ex) {
            Exceptions.printStackTrace(ex);
        }
    }
    /**
     * Java does not support unsigned ints
     * Compares two ints as if they were unsigned ints
     *
     * @param a
     * @param b
     * @return true if a > b
     */
    public static boolean unsignedIsGreater(int a, int b) {
        long aLong = a & 0xFFFFFFFFL;
        long bLong = b & 0xFFFFFFFFL;

        return aLong > bLong;
    }
    /**
     * Java does not support unsigned ints
     * Compares two ints as if they were unsigned ints
     *
     * @param a
     * @param b
     * @return true if a >= b
     */
    public static boolean unsignedIsGreaterOrEqual(int a, int b) {
        long aLong = a & 0xFFFFFFFFL;
        long bLong = b & 0xFFFFFFFFL;

        return aLong >= bLong;
    }
    /**
     * Java does not support unsigned ints
     * Compares two ints as if they were unsigned ints
     *
     * @param a
     * @param b
     * @return true if a < b
     */
    public static boolean unsignedIsLess(int a, int b) {
        long aLong = a & 0xFFFFFFFFL;
        long bLong = b & 0xFFFFFFFFL;

        return aLong < bLong;
    }
    /**
     * Java does not support unsigned ints
     * Compares two ints as if they were unsigned ints
     *
     * @param a
     * @param b
     * @return true if a <= b
     */
    public static boolean unsignedIsLessOrEqual(int a, int b) {
        long aLong = a & 0xFFFFFFFFL;
        long bLong = b & 0xFFFFFFFFL;

        return aLong <= bLong;
    }
    /**
     * Go from little endian to big endian or viceverza
     *
     * @param i
     * @return flipped value
     */
    public static int flipInt(int i) {
        int res = (int) (i >>> 24);
        res |= (i >> 8)  & 0x0000ff00;
        res |= (i << 8)  & 0x00ff0000;
        res |= (i & 0xff) << 24;
        return res;
    }
    /**
     * Go from little endian to big endian or viceverza
     *
     * @param s
     * @return flipped value
     */
    public static short flipShort(short s) {
        short res = (short) (s >>> 8);
        res |= (s << 8);
        return res;
    }
    /**
     * Just like memset in C but the size is assumed to be the whole array
     * 
     * @param array
     * @param value
     */
    public static void fillArray(byte [] array, byte value) {
        for(int i=0; i<array.length;++i)
            array[i] = value;
    }
    /**
     * Erase the 4th byte. Handy when you create a random buffer for a
     * 16 bit part (16 bit because they have 16 bit data witdh but 24 bit opcodes)
     * and want to remove the 'unused' byte. MPLAB X memory objects for these
     * parts use 32 bits for one opcode. But in the HW tools return 24 bit buffers.
     * If you write the random
     * buffer to target and read it back, the 4th byte will be zero out and your
     * comparison to the original random buffer would fail (unless you call this
     * function on the original random buffer first).
     * @param array
     */
    public static void clear4thByte(byte [] array) {
        int j = 0;
        for (int i = 0 ; i < array.length ; ++i) {
            if (++j == 4) {
                array[i] = 0;
                j = 0;
            }
        }
    }
     /**
      * Compare two byte buffers
      * @param b1
      * @param b2
      * @return first entry in pair is true if buffers match, fals if not
      *         second entry returns "" when buffers are the same or the
      *         reason buffers are different
      */
    public static Pair<Boolean, String> compareBuffers(byte [] b1, byte [] b2) {
        boolean ok = true;
        StringBuilder reason  = new StringBuilder("");
        int max=10;
        int bad=0;


        if (b1.length != b2.length) {
            reason.append(String.format("buffers are not the same length (0x%X versus 0x%X)", b1.length, b2.length));
            ok = false;
            return new Pair<Boolean, String>(ok, reason.toString());
        }
        for (int i = 0; i < b1.length ; ++i ) {
            if (b1[i] != b2[i]) {
                ok = false;
                reason.append(String.format("Error at index %d expected 0x%X, got 0x%X\n", i, b1[i], b2[i]));
                if (bad++ > maxErrors) {
                    reason.append(String.format("Only the first %d errors are reported.\n", maxErrors));
                    break;
                }
            }
        }
        return new Pair<Boolean, String>(ok, reason.toString());
    }
    /**
     * Copy the data from src into a caller allocated int [] buffer
     * This function will not do anything unless the src len is
     * 4 times the size of dst len.
     * This function assume little endian format in src
     * @param dst
     */
    public static void convertBuffer(byte[] src, int dst[]) {
        assert src.length  == dst.length * 4;
        int index = 0;
        if (src.length == dst.length * 4) {
            int val;
            for(int j = 0; j < dst.length; ++j) {
                val  =    (int)src[index++]         & 0x000000ff;
                val += ((((int)src[index++]) <<  8) & 0x0000ff00);
                val += ((((int)src[index++]) << 16) & 0x00ff0000);
                val += ((((int)src[index++]) << 24) & 0xff000000);
                dst[j] = val;
            }
        }
    }
    /**
     * If length of src is not a multple of 4 fail returning null
     * Otherwise, create a buffer of ints the same overall size as src.length
     * in bytes and copy the data from src into it. Return the value
     * @param src array of bytes to be copied into array of ints
     * @return null or newly created array of ints with the same data as src
     */
    public static int [] convertBuffer(byte [] src) {
        int [] res = null;
        if (src.length % 4 == 0) {
            res = new int[src.length/4];
            convertBuffer(src, res);
        }
        return res;
    }
    /**
     * Copy the data from src into a caller allocated int [] buffer
     * This function will not do anything unless the src len is
     * 4 times the size of dst len.
     * This function assume little endian format in src
     * @param dst
     */
    public static void convertBuffer(int[] src, byte dst[]) {
        assert dst.length  == src.length * 4;
        int index = 0;
        if (dst.length == src.length * 4) {
            int val;
            for(int j = 0; j < src.length; ++j) {
                val = src[j];
                dst[index++] = (byte) (val        & 0x000000ff);
                dst[index++] = (byte)((val >>  8) & 0x000000ff);
                dst[index++] = (byte)((val >> 16) & 0x000000ff);
                dst[index++] = (byte)((val >> 24) & 0x000000ff);
            }
        }
    }
    /**
     * Create a buffer of bytes src.length*4 long
     * and copy the data from src into it. Return the value
     * @param src array of ints to be copied into array of bytes
     * @return null or newly created array of ints with the same data as src
     */
    public static byte [] convertBuffer(int [] src) {
        byte [] res = new byte[src.length*4];
        convertBuffer(src, res);
        return res;
    }
}
