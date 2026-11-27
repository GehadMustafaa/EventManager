#pragma once
#include <QDialog>
#include <QString>

class FormField;
class QComboBox;
class QDateEdit;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;

class EventDialog : public QDialog {
public:
    explicit EventDialog(QWidget* parent = nullptr);
    EventDialog(const QString& name, const QString& date, const QString& time, const QString& venue, int capacity,
                const QString& description = QString(), QWidget* parent = nullptr);

    QString name() const;
    QString date() const;
    QString time() const;
    QString venue() const;
    int capacity() const;
    QString description() const;

    // For problems only the core can detect (for example a duplicate name). The dialog stays open.
    void showNameError(const QString& message);

private:
    bool validate();

    QLineEdit* name_;
    QDateEdit* date_;
    QSpinBox* hour_;
    QSpinBox* minute_;
    QComboBox* period_;
    QLineEdit* venue_;
    QSpinBox* capacity_;
    QPlainTextEdit* description_;
    FormField* nameField_;
    FormField* venueField_;
    QLabel* titleLabel_;
    QLabel* leadLabel_;
    QPushButton* submitButton_;
};
