#pragma once
#include <QFrame>
#include <QString>
#include <functional>

class QLabel;
class QPushButton;
class QTimer;

class ActionToast : public QFrame {
    Q_OBJECT

public:
    explicit ActionToast(QWidget* parent);

    void showMessage(const QString& text, const QString& actionLabel = QString(),
                     std::function<void()> onAction = {}, const QString& shortcutHint = QString());
    void reposition();

protected:
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    QLabel* textLabel_ = nullptr;
    QLabel* shortcutLabel_ = nullptr;
    QPushButton* actionButton_ = nullptr;
    QPushButton* closeButton_ = nullptr;
    QTimer* timer_ = nullptr;
    std::function<void()> onAction_;
};
