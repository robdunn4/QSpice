#ifndef ACTIONBUTTON_H
#define ACTIONBUTTON_H

#include <QAction>
#include <QPushButton>

/**
 * @brief An extension of a QPushButton that supports QAction.
 *
 * This class represents a QPushButton extension that can be connected to an
 * action and that configures itself depending on the status of the action. When
 * the action changes its state, the button reflects such changes, and when the
 * button is clicked the action is triggered.
 * @note This code is derived from
 * https://wiki.qt.io/PushButton_Based_On_Action.
 */
class ActionButton : public QPushButton {
  Q_OBJECT
private:
  QAction *actionOwner = nullptr; ///< Action associated with button.

public:
  /**
   * @brief Basic constructor.
   * @param parent Parent widget.
   */
  explicit ActionButton(QWidget *parent = nullptr);

  /**
   * @brief Alternate constructor.
   * @param action Action to associate with button.
   * @param parent Parent widget.
   */
  ActionButton(QAction *action, QWidget *parent = nullptr);

  /**
   * @brief Set the action associated with the button.
   *
   * The button is configured immediately depending on the action status and the
   * button and the action are connected together so that when the action is
   * changed the button is updated and when the button is clicked the action is
   * triggered.
   *
   * @param action The action to associate with this button
   */
  void setAction(QAction *action);

public slots:
  /**
   * @brief Update the button status depending on a change in the action status.
   *
   * This slot is invoked each time the action "changed" signal is emitted.
   */
  void updateButtonStatusFromAction();
};

#endif // ACTIONBUTTON_H
