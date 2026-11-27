#include "ActionToast.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include "Theme.h"

ActionToast::ActionToast(QWidget* parent) : QFrame(parent), timer_(new QTimer(this)) {
    setObjectName("toastPill");
    hide();

    textLabel_ = new QLabel;
    textLabel_->setWordWrap(false);

    shortcutLabel_ = new QLabel;
    shortcutLabel_->setProperty("role", "kbd");
    shortcutLabel_->setFont(Theme::monoFont(11));
    shortcutLabel_->hide();

    actionButton_ = new QPushButton;
    actionButton_->setProperty("variant", "ghost");
    actionButton_->setFocusPolicy(Qt::TabFocus);
    actionButton_->hide();

    closeButton_ = new QPushButton(QString::fromUtf8("\xC3\x97"));
    closeButton_->setProperty("variant", "icon");
    closeButton_->setFixedSize(26, 26);
    closeButton_->setAccessibleName("Dismiss notification");
    closeButton_->setFocusPolicy(Qt::TabFocus);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(Theme::space4, Theme::space2, Theme::space2, Theme::space2);
    layout->setSpacing(Theme::space2);
    layout->addWidget(textLabel_);
    layout->addWidget(shortcutLabel_);
    layout->addWidget(actionButton_);
    layout->addWidget(closeButton_);

    timer_->setSingleShot(true);
    connect(timer_, &QTimer::timeout, this, &QWidget::hide);
    connect(closeButton_, &QPushButton::clicked, this, &QWidget::hide);
    connect(actionButton_, &QPushButton::clicked, this, [this] {
        auto cb = std::move(onAction_);
        hide();
        if (cb) cb();
    });
}

void ActionToast::showMessage(const QString& text, const QString& actionLabel,
                              std::function<void()> onAction, const QString& shortcutHint) {
    textLabel_->setText(text);
    onAction_ = std::move(onAction);

    const bool hasAction = !actionLabel.isEmpty() && static_cast<bool>(onAction_);
    actionButton_->setText(actionLabel);
    actionButton_->setVisible(hasAction);

    shortcutLabel_->setText(shortcutHint);
    shortcutLabel_->setVisible(!shortcutHint.isEmpty());

    adjustSize();
    reposition();
    show();
    raise();
    timer_->start(6000);
}

void ActionToast::reposition() {
    if (!parentWidget()) return;
    adjustSize();
    const QRect area = parentWidget()->rect();
    const int maxW = qMax(260, area.width() - Theme::space6 * 2);
    if (width() > maxW) setFixedWidth(maxW);
    else setMaximumWidth(maxW);
    move(area.right() - width() - Theme::space4, area.bottom() - height() - Theme::space4);
}

void ActionToast::enterEvent(QEnterEvent* event) {
    timer_->stop();
    QFrame::enterEvent(event);
}

void ActionToast::leaveEvent(QEvent* event) {
    if (isVisible()) timer_->start(3500);
    QFrame::leaveEvent(event);
}
