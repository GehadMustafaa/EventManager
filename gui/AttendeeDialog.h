#pragma once
#include <QDialog>
#include <QString>

class FormField;
class QLineEdit;

class QLabel;
class QPushButton;

class AttendeeDialog : public QDialog {
public:
    explicit AttendeeDialog(QWidget* parent = nullptr);
    AttendeeDialog(const QString& name, const QString& email, QWidget* parent = nullptr);

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
    QLabel* titleLabel_;
    QLabel* leadLabel_;
    QPushButton* submitButton_;
};
