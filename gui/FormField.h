#pragma once
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include "Theme.h"

// A label above an input, with an inline error line underneath (hidden until needed).
class FormField : public QWidget {
public:
    FormField(const QString& label, QWidget* input, QWidget* parent = nullptr) : QWidget(parent), input_(input) {
        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(Theme::space1);

        auto* caption = new QLabel(label);
        caption->setProperty("role", "fieldLabel");
        caption->setBuddy(input);

        hint_ = new QLabel;
        hint_->setProperty("role", "caption");
        hint_->setWordWrap(true);
        hint_->hide();

        error_ = new QLabel;
        error_->setProperty("role", "error");
        error_->setWordWrap(true);
        error_->hide();

        layout->addWidget(caption);
        layout->addWidget(input);
        layout->addWidget(hint_);
        layout->addWidget(error_);
    }

    void setHint(const QString& message) {
        hint_->setText(message);
        hint_->setVisible(!message.isEmpty());
    }

    void setError(const QString& message) {
        error_->setText(message);
        error_->setVisible(!message.isEmpty());
        input_->setProperty("invalid", !message.isEmpty());
        input_->setAccessibleDescription(message);
        Theme::repolish(input_);
    }

    bool hasError() const { return !error_->isHidden(); }

private:
    QWidget* input_;
    QLabel* hint_;
    QLabel* error_;
};
