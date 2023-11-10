/*==============================================================================
 * GPIO_Class.h -- QSpice C-Block interface class for GPIO pins (GPIO_Pin.qsym).
 *
 * Requirements:  Input and output ports may be any data type but must be of
 * same data type.  The reference voltage port data type must be double.  The
 * control voltatge port data type must be boolean.
 *
 * Note:  This file is used for both declaration and implementation of the
 * class. Using a header file to do both is generally a bad coding practice but
 * simplifies the compile/link configuration.
 *
 * Note:  Does not compile with DMC.  Requires C++17 or newer.
 *============================================================================*/
#ifndef GPIO_CLASS_H
#define GPIO_CLASS_H

#include <memory>

/*------------------------------------------------------------------------------
 * I'm following Microchip PIC uC conventions for TRIS direction and including a
 * latch.  When you write to the port, you also set/clear the latch.  When you
 * read from the port, you get the (possibly loaded) value/state present on the
 * port. When you read from the latch, you get the actual "write state," i.e.,
 * the state that the GPIO is trying to apply to the output.
 *----------------------------------------------------------------------------*/
#define TRIS_OUT 0
#define TRIS_IN  1

/*------------------------------------------------------------------------------
 * GPIO_Pin class -- virtual base class for GPIO pins.
 *----------------------------------------------------------------------------*/
class GPIO_Pin {
public:
  GPIO_Pin() {}

  virtual ~GPIO_Pin() {}

  virtual void setTris(bool dir) = 0;

  virtual bool readPort() = 0;

  virtual bool readLatch() = 0;

  virtual void writePort(bool state) = 0;
};

/*------------------------------------------------------------------------------
 * GPIO_Pin_T class template -- derived from GPIO_Pin class.  Provides overloads
 * based on pin data type.
 *----------------------------------------------------------------------------*/
template <typename T> class GPIO_Pin_T : public GPIO_Pin {
public:
  GPIO_Pin_T(const T &inPort, T &outPort, bool &ctlPort, const double &refPort)
      : inPort(inPort), outPort(outPort), ctlPort(ctlPort), refPort(refPort) {
    setTris(TRIS_IN);
  }

  ~GPIO_Pin_T() {}

  void setTris(bool dir) override {
    tris    = dir;
    ctlPort = dir;
  }

  bool readPort() override {
    // revisit to specialize for boolean?
    return double(inPort) > (refPort / 2.0);
  }

  bool readLatch() override { return latch; }

  void writePort(bool state) override {
    latch   = state;
    outPort = state * refPort;
  }

protected:
  const T      &inPort;
  T            &outPort;
  bool         &ctlPort;
  const double &refPort;
  bool          tris;
  bool          latch;
};

/*------------------------------------------------------------------------------
 * makeGpioPinPtr() template -- convenience function for constructing a
 * GPIO_Pin-derived instance and returning a unique_ptr.
 *----------------------------------------------------------------------------*/
typedef std::unique_ptr<GPIO_Pin> GPIO_Pin_Ptr;

template <typename T>
GPIO_Pin_Ptr makeGpioPinPtr(
    const T &inPort, T &outPort, bool &ctlPort, const double &refPort) {
  return GPIO_Pin_Ptr(new GPIO_Pin_T(inPort, outPort, ctlPort, refPort));
}

#endif GPIO_CLASS_H
/*==============================================================================
 * End of GPIO_Class.h
 *============================================================================*/