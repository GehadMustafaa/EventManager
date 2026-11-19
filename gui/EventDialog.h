#pragma once
#include <QDialog>
#include <QString>

class FormField;
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

    // For problems only the core can detect (for example a duplicate name). The dialog stays open.
    void showNameError(const QString& message);

private:
    bool validate();

    QLineEdit* name_;
    QDateEdit* date_;
    QLineEdit* venue_;
    QSpinBox* capacity_;
    FormField* nameField_;
    FormField* venueField_;
};
