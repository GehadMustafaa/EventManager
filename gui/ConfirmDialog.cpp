#include "ConfirmDialog.h"
#include "Theme.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

ConfirmDialog::ConfirmDialog(const QString &title, const QString &body,
                             const QString &confirmLabel,
                             const QString &cancelLabel, bool destructive,
                             const QString &secondaryLabel, QWidget *parent)
    : QDialog(parent) {
  setWindowTitle(title);
  setWindowFlag(Qt::WindowContextHelpButtonHint, false);
  setFixedWidth(440);

  auto *heading = new QLabel(title);
  heading->setFont(Theme::titleFont(18));
  heading->setWordWrap(true);

  auto *message = new QLabel(body);
  message->setProperty("role", "muted");
  message->setWordWrap(true);

  auto *buttons = new QHBoxLayout;
  buttons->setSpacing(Theme::space2);
  buttons->addStretch();

  QPushButton *cancelBtn = nullptr;
  if (!cancelLabel.isEmpty()) {
    cancelBtn = new QPushButton(cancelLabel);
    cancelBtn->setFocusPolicy(Qt::TabFocus);
    connect(cancelBtn, &QPushButton::clicked, this, [this] {
      result_ = Result::Cancel;
      reject();
    });
    buttons->addWidget(cancelBtn);
  }

  if (!secondaryLabel.isEmpty()) {
    auto *secBtn = new QPushButton(secondaryLabel);
    secBtn->setProperty("variant", "danger");
    secBtn->setFocusPolicy(Qt::TabFocus);
    connect(secBtn, &QPushButton::clicked, this, [this] {
      result_ = Result::Secondary;
      accept();
    });
    buttons->addWidget(secBtn);
  }

  auto *confirmBtn = new QPushButton(confirmLabel);
  confirmBtn->setProperty("variant", destructive ? "danger" : "primary");
  confirmBtn->setFocusPolicy(Qt::TabFocus);
  confirmBtn->setDefault(!destructive || !cancelBtn);
  confirmBtn->setAutoDefault(!destructive || !cancelBtn);
  connect(confirmBtn, &QPushButton::clicked, this, [this] {
    result_ = Result::Primary;
    accept();
  });
  buttons->addWidget(confirmBtn);

  if (destructive && cancelBtn) {
    safeDefaultBtn_ = cancelBtn;
    cancelBtn->setDefault(true);
    cancelBtn->setAutoDefault(true);
    cancelBtn->setProperty("kbdFocus", true);
    Theme::repolish(cancelBtn);
    cancelBtn->setFocus(Qt::TabFocusReason);
  }

  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(Theme::space6, Theme::space6, Theme::space6,
                             Theme::space6);
  layout->setSpacing(Theme::space3);
  layout->addWidget(heading);
  layout->addWidget(message);
  layout->addSpacing(Theme::space2);
  layout->addLayout(buttons);
}

void ConfirmDialog::showEvent(QShowEvent *event) {
  QDialog::showEvent(event);
  if (safeDefaultBtn_) {
    safeDefaultBtn_->setDefault(true);
    safeDefaultBtn_->setProperty("kbdFocus", true);
    Theme::repolish(safeDefaultBtn_);
    safeDefaultBtn_->setFocus(Qt::TabFocusReason);
  }
}

bool ConfirmDialog::ask(QWidget *parent, const QString &title,
                        const QString &body, const QString &confirmLabel,
                        const QString &cancelLabel, bool destructive) {
  ConfirmDialog dlg(title, body, confirmLabel, cancelLabel, destructive,
                    QString(), parent);
  return dlg.exec() == QDialog::Accepted && dlg.result() == Result::Primary;
}

void ConfirmDialog::inform(QWidget *parent, const QString &title,
                           const QString &body) {
  ConfirmDialog dlg(title, body, QStringLiteral("Dismiss"), QString(), false,
                    QString(), parent);
  dlg.exec();
}
