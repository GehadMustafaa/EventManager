#pragma once
#include <QDialog>
#include <QString>

class QDateEdit;
class QLineEdit;
class QSpinBox;

class EventDialog : public QDialog {
public:
    explicit EventDialog(QWidget* parent = nullptr);

    QString name() const;
    QString date() const;
    QString venue() const;
    int capacity() const;

private:
    QLineEdit* name_;
    QDateEdit* date_;
    QLineEdit* venue_;
    QSpinBox* capacity_;
};
