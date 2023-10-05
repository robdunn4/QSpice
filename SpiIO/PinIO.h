/*==============================================================================
 * PinIO.h -- Classes for digital input/output pin state management and edge
 * detection.
 *============================================================================*/

#ifndef PINIO_H
#define PINIO_H

enum PinState { LOW, HIGH };
enum PinEdge { NONE, FALLING, RISING, IGNORE };

/*------------------------------------------------------------------------------
 * PinBase class -- class to manage output pin states.
 *----------------------------------------------------------------------------*/
class PinBase {
public:
  PinBase(PinState state = PinState::LOW, PinEdge edge = PinEdge::NONE)
      : state(state), edge(edge) {}

  PinState getState() const { return state; }

  PinEdge getEdge() const { return edge; }

protected:
  PinState setState(PinState toState) {
    if (state == toState) edge = PinEdge::NONE;
    else edge = toState == PinState::HIGH ? PinEdge::RISING : PinEdge::FALLING;
    state = toState;
    return state;
  }

  PinState setState(bool toState) {
    return setState(toState ? PinState::HIGH : PinState::LOW);
  }

  PinState toggleState() {
    return setState(state == PinState::HIGH ? PinState::LOW : PinState::HIGH);
  }

public:
  // convenience methods
  inline bool isHigh() const { return state == PinState::HIGH; }
  inline bool isLow() const { return !isHigh(); }
  inline bool isEdge() const { return edge != PinEdge::NONE; }
  inline bool isRising() const { return edge == PinEdge::RISING; }
  inline bool isFalling() const { return edge == PinEdge::FALLING; }

protected:
  PinState setHigh() { return setState(PinState::HIGH); }
  PinState setLow() { return setState(PinState::LOW); }

protected:
  PinState state;
  PinEdge  edge;
};

/*------------------------------------------------------------------------------
 * PinOut class -- class to manage output pin states.
 *
 * This class saves the current state and can return the current state as a
 * digital "high" or "low" voltage.
 *----------------------------------------------------------------------------*/
class PinOut : public PinBase {
public:
  PinOut() : PinBase() {}
  PinOut(double vcc, PinState idleState) : PinBase(idleState) {
    init(vcc, idleState);
  }

protected:
  void init(double vcc, PinState idleState) {
    vHigh           = vcc;
    this->idleState = idleState;
  }

public:
  double getStateV() const { return state == PinState::HIGH ? vHigh : 0.0; }

  // returns reference for chaining
  PinOut &setIdle() {
    setState(idleState);
    return *this;
  }

  // returns reference for chaining
  PinOut &setHigh() {
    PinBase::setState(PinState::HIGH);
    return *this;
  }

  // returns reference for chaining
  PinOut &setLow() {
    PinBase::setState(PinState::LOW);
    return *this;
  }

  // returns reference for chaining
  PinOut &setState(PinState toState) {
    if (state == toState) edge = PinEdge::NONE;
    else edge = toState == PinState::HIGH ? PinEdge::RISING : PinEdge::FALLING;
    state = toState;
    return *this;
  }

  // returns reference for chaining
  PinOut &setState(bool toState) {
    PinBase::setState(toState ? PinState::HIGH : PinState::LOW);
    return *this;
  }

  // returns reference for chaining
  PinOut &toggleState() {
    PinBase::toggleState();
    return *this;
  }

protected:
  double   vHigh;
  PinState idleState;
};

/*------------------------------------------------------------------------------
 * PinIn class -- class to manage input pin states.
 *
 * This class saves the current pin state and, when changed, also saves the
 * transition information (i.e., that the last state change was a rising/falling
 * edge or no change/not an edge transition).
 *
 * Note:  It would be easy to add hysteresis to this class if that would be
 * useful.
 *----------------------------------------------------------------------------*/
class PinIn : public PinBase {
public:
  PinIn() : PinBase() {}
  PinIn(double vcc, double vIn) : PinBase() { init(vcc, vIn); }

protected:
  void init(double vcc, double vIn) {
    halfVcc = vcc / 2.0;
    setState(vIn);
  }

public:
  PinIn &setState(double vPin) {
    PinBase::setState(
        PinState(vPin > halfVcc ? PinState::HIGH : PinState::LOW));
    return *this;
  }

protected:
  double halfVcc = 0.0;
};

#endif   // PINIO_H
/*==============================================================================
 * End of PinIO.h
 *============================================================================*/