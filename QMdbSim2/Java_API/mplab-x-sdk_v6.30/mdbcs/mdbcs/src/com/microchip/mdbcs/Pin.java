/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.microchip.mdbcs;

/**
 * Simplified version of the com.microchip.mplab.mdbcore.simulator.Pin interface 
 */
public interface Pin {
    
    public double getVoltage();
    public void setVoltage(double volt);
    public void externalSetVoltage(double volt);
    public boolean getValue();
    public void setValue(boolean val);
    public void externalSetValue(boolean val);
    public boolean isAnalog();
    public boolean isDigital();
    public boolean isDigitalInput();
    public boolean isDigitalOutput();
    
}
