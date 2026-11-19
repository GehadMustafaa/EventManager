#include "AttendeeDialog.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include "FormField.h"
#include "Theme.h"

namespace {
bool hasInvalidChars(const QString& value) {
    return value.contains('|') || value.contains('\n');
}
}

AttendeeDialog::AttendeeDialog(QWidget* parent) : QDialog(parent), name_(new QLineEdit), email_(new QLineEdit) {
    setWindowTitle("Register Attendee");
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setFixedWidth(440);

    name_->setPlaceholderText("Ada Lovelace");
    email_->setPlaceholderText("ada@example.com");

    auto* title = new QLabel("Register Attendee");
    title->setFont(Theme::titleFont(20));
    auto* lead = new QLabel("Attendees are identified by email, so each one can be registered once.");
    lead->setProperty("role", "muted");
    lead->setWordWrap(true);

    nameField_ = new FormField("Name", name_);
    emailField_ = new FormField("Email", email_);

    auto* cancel = new QPushButton("Cancel");
    cancel->setFocusPolicy(Qt::TabFocus);
    auto* submit = new QPushButton("Register Attendee");
    submit->setProperty("variant", "primary");
    submit->setFocusPolicy(Qt::TabFocus);
    submit->setDefault(true);
    auto* buttons = new QHBoxLayout;
    buttons->setSpacing(Theme::space2);
    buttons->addStretch();
    buttons->addWidget(cancel);
    buttons->addWidget(submit);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(Theme::space6, Theme::space6, Theme::space6, Theme::space6);
    layout->setSpacing(Theme::space4);
    layout->addWidget(title);
    layout->addWidget(lead);
    layout->addSpacing(Theme::space1);
    layout->addWidget(nameField_);
    layout->addWidget(emailField_);
    layout->addSpacing(Theme::space2);
    layout->addLayout(buttons);

    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(submit, &QPushButton::clicked, this, [this] { if (validate()) accept(); });
    connect(name_, &QLineEdit::textChanged, this, [this] { if (nameField_->hasError()) nameField_->setError(QString()); });
    connect(email_, &QLineEdit::textChanged, this, [this] { if (emailField_->hasError()) emailField_->setError(QString()); });
}

bool AttendeeDialog::validate() {
    QString nameProblem;
    if (name().isEmpty()) nameProblem = "Enter the attendee's name";
    else if (hasInvalidChars(name())) nameProblem = "Remove any | characters and line breaks";

    QString emailProblem;
    const int at = email().indexOf('@');
    if (email().isEmpty()) emailProblem = "Enter an email address";
    else if (hasInvalidChars(email())) emailProblem = "Remove any | characters and line breaks";
    else if (at < 1 || at == email().size() - 1) emailProblem = "Enter an address like name@example.com";

    nameField_->setError(nameProblem);
    emailField_->setError(emailProblem);
    if (!nameProblem.isEmpty()) name_->setFocus();
    else if (!emailProblem.isEmpty()) email_->setFocus();
    return nameProblem.isEmpty() && emailProblem.isEmpty();
}

void AttendeeDialog::showEmailError(const QString& message) {
    emailField_->setError(message);
    email_->setFocus();
    email_->selectAll();
}

QString AttendeeDialog::name() const { return name_->text().trimmed(); }
QString AttendeeDialog::email() const { return email_->text().trimmed(); }
