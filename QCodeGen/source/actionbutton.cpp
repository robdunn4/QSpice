#include "actionbutton.h"

ActionButton::ActionButton(QWidget *parent) : QPushButton(parent) {}

ActionButton::ActionButton(QAction *action, QWidget *parent)
    : QPushButton(parent) {
  Q_ASSERT(action);
  setAction(action);
}

void ActionButton::setAction(QAction *action) {
  // if there is already an action associated with the button, remove all
  // connections
  if (actionOwner && actionOwner != action) {
    disconnect(actionOwner, &QAction::changed, this,
        &ActionButton::updateButtonStatusFromAction);
    disconnect(this, &ActionButton::clicked, actionOwner, &QAction::trigger);
  }

  // store the action
  actionOwner = action;

  // configure the button
  updateButtonStatusFromAction();

  // connect the action and the button so that, when the action is changed, the
  // button is also changed
  connect(action, &QAction::changed, this,
      &ActionButton::updateButtonStatusFromAction);

  // connect the button to the slot that forwards the signal to the action
  connect(this, &ActionButton::clicked, actionOwner, &QAction::trigger);
}

void ActionButton::updateButtonStatusFromAction() {
  if (!actionOwner) return;
  setText(actionOwner->text());
  setStatusTip(actionOwner->statusTip());
  setToolTip(actionOwner->toolTip());
  setIcon(actionOwner->icon());
  setEnabled(actionOwner->isEnabled());
  setCheckable(actionOwner->isCheckable());
  setChecked(actionOwner->isChecked());
}
