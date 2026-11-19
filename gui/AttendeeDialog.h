#pragma once
#include <QDialog>
#include <QString>

class FormField;
class QLineEdit;

class AttendeeDialog : public QDialog {
public:
    explicit AttendeeDialog(QWidget* parent = nullptr);

    QString name() const;
    QString email() const;

    // For problems only the core can detect (for example an email that is already registered).
    void showEmailError(const QString& message);

private:
    bool validate();

    QLineEdit* name_;
    QLineEdit* email_;
    FormField* nameField_;
    FormField* emailField_;
};
