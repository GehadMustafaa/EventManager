#include "EventDialog.h"
#include <QDate>
#include <QDateEdit>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include "FormField.h"
#include "Theme.h"

namespace {
QString textProblem(const QString& value, const QString& emptyMessage) {
    if (value.isEmpty()) return emptyMessage;
    if (value.contains('|') || value.contains('\n')) return QStringLiteral("Remove any | characters and line breaks");
    return QString();
}
}

EventDialog::EventDialog(QWidget* parent)
    : QDialog(parent),
      name_(new QLineEdit),
      date_(new QDateEdit(QDate::currentDate())),
      venue_(new QLineEdit),
      capacity_(new QSpinBox) {
    setWindowTitle("New Event");
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setFixedWidth(440);

    date_->setCalendarPopup(true);
    date_->setDisplayFormat("yyyy-MM-dd");
    capacity_->setRange(1, 100000);
    capacity_->setValue(50);
    capacity_->setButtonSymbols(QAbstractSpinBox::NoButtons);
    name_->setPlaceholderText("Spring Gala");
    venue_->setPlaceholderText("Main Hall");

    auto* title = new QLabel("New Event");
    title->setFont(Theme::titleFont(20));
    auto* lead = new QLabel("Add an event that attendees can book into.");
    lead->setProperty("role", "muted");

    nameField_ = new FormField("Name", name_);
    venueField_ = new FormField("Venue", venue_);
    auto* dateField = new FormField("Date", date_);
    auto* capacityField = new FormField("Capacity (seats)", capacity_);

    auto* row = new QHBoxLayout;
    row->setSpacing(Theme::space4);
    row->addWidget(dateField, 1);
    row->addWidget(capacityField, 1);

    auto* cancel = new QPushButton("Cancel");
    cancel->setFocusPolicy(Qt::TabFocus);
    auto* create = new QPushButton("Create Event");
    create->setProperty("variant", "primary");
    create->setFocusPolicy(Qt::TabFocus);
    create->setDefault(true);
    auto* buttons = new QHBoxLayout;
    buttons->setSpacing(Theme::space2);
    buttons->addStretch();
    buttons->addWidget(cancel);
    buttons->addWidget(create);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(Theme::space6, Theme::space6, Theme::space6, Theme::space6);
    layout->setSpacing(Theme::space4);
    layout->addWidget(title);
    layout->addWidget(lead);
    layout->addSpacing(Theme::space1);
    layout->addWidget(nameField_);
    layout->addWidget(venueField_);
    layout->addLayout(row);
    layout->addSpacing(Theme::space2);
    layout->addLayout(buttons);

    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(create, &QPushButton::clicked, this, [this] { if (validate()) accept(); });
    connect(name_, &QLineEdit::textChanged, this, [this] { if (nameField_->hasError()) nameField_->setError(QString()); });
    connect(venue_, &QLineEdit::textChanged, this, [this] { if (venueField_->hasError()) venueField_->setError(QString()); });
}

bool EventDialog::validate() {
    const QString nameProblem = textProblem(name(), "Enter a name for the event");
    const QString venueProblem = textProblem(venue(), "Enter where the event takes place");
    nameField_->setError(nameProblem);
    venueField_->setError(venueProblem);
    if (!nameProblem.isEmpty()) name_->setFocus();
    else if (!venueProblem.isEmpty()) venue_->setFocus();
    return nameProblem.isEmpty() && venueProblem.isEmpty();
}

void EventDialog::showNameError(const QString& message) {
    nameField_->setError(message);
    name_->setFocus();
    name_->selectAll();
}

QString EventDialog::name() const { return name_->text().trimmed(); }
QString EventDialog::date() const { return date_->date().toString("yyyy-MM-dd"); }
QString EventDialog::venue() const { return venue_->text().trimmed(); }
int EventDialog::capacity() const { return capacity_->value(); }
