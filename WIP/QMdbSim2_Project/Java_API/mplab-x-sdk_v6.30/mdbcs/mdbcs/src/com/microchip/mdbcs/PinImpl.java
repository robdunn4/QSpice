/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package com.microchip.mdbcs;

import com.microchip.mplab.mdbcore.simulator.Pin.PinState;

/**
 * It is package protected. No public access since it uses the actual
 * mdbcore sim pin interface. Exposes the our simplified version of Pin
 */
class PinImpl implements Pin {
    private com.microchip.mplab.mdbcore.simulator.Pin pin;
    
    public PinImpl(com.microchip.mplab.mdbcore.simulator.Pin p) {
        pin = p;
    }
    @Override
    public double getVoltage() {
        return pin.getAnalogValue();
    }

    @Override
    public void setVoltage(double val) {
        pin.setAnalogValue(val);
    }
    
     /**
     * This method is used to set the input voltage for the correspond input pin.
     * 
     * @param val the input voltage of this input pin
     * 
     */
    @Override
    public void externalSetVoltage(double val) {
        pin.externalSetAnalogValue(val);
    }

    @Override
    public boolean getValue() {
        return pin.get().equals(com.microchip.mplab.mdbcore.simulator.Pin.PinState.HIGH);
    }

    @Override
    public void setValue(boolean val) {
        pin.set(val ? com.microchip.mplab.mdbcore.simulator.Pin.PinState.HIGH : com.microchip.mplab.mdbcore.simulator.Pin.PinState.LOW);
    }
    
    /**
     * This method is used to set the pin state of the correspond input pin.
     * 
     * @param val the pin state of this input pin
     * 
     */
    @Override
    public void externalSetValue(boolean val) {
        pin.externalSet(val ? com.microchip.mplab.mdbcore.simulator.Pin.PinState.HIGH : com.microchip.mplab.mdbcore.simulator.Pin.PinState.LOW);
    }

    @Override
    public boolean isAnalog() {
        return pin.getADState().equals(com.microchip.mplab.mdbcore.simulator.Pin.ADState.ANALOG);
    }

    @Override
    public boolean isDigital() {
        return pin.getADState().equals(com.microchip.mplab.mdbcore.simulator.Pin.ADState.DIGITAL);
    }

    @Override
    public boolean isDigitalInput() {
        boolean res = false;
        if (isDigital()) {
            res = pin.getIOState().equals(com.microchip.mplab.mdbcore.simulator.Pin.IOState.INPUT);
        }
        return res;
    }

    @Override
    public boolean isDigitalOutput() {
        boolean res = false;
        if (isDigital()) {
            res = pin.getIOState().equals(com.microchip.mplab.mdbcore.simulator.Pin.IOState.OUTPUT);
        }
        return res;
    }
    
}
