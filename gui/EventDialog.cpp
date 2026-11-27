#include "EventDialog.h"
#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include "DateUtil.h"
#include "FormField.h"
#include "Theme.h"

namespace {

class ZeroPadSpinBox : public QSpinBox {
public:
    using QSpinBox::QSpinBox;
    QString textFromValue(int value) const override {
        return QStringLiteral("%1").arg(value, 2, 10, QLatin1Char('0'));
    }
};

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
      hour_(new ZeroPadSpinBox),
      minute_(new ZeroPadSpinBox),
      period_(new QComboBox),
      venue_(new QLineEdit),
      capacity_(new QSpinBox),
      description_(new QPlainTextEdit) {
    setWindowTitle("New Event");
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setFixedWidth(460);

    date_->setCalendarPopup(true);
    date_->setDisplayFormat("yyyy-MM-dd");
    hour_->setRange(1, 12);
    hour_->setValue(9);
    hour_->setWrapping(true);
    hour_->setAlignment(Qt::AlignCenter);
    hour_->setAccessibleName("Hour");
    hour_->setToolTip("Hour (1-12)");
    minute_->setRange(0, 59);
    minute_->setValue(0);
    minute_->setWrapping(true);
    minute_->setAlignment(Qt::AlignCenter);
    minute_->setAccessibleName("Minute");
    minute_->setToolTip("Minute (00-59)");
    period_->addItems({"AM", "PM"});
    period_->setAccessibleName("AM or PM");
    period_->setToolTip("AM or PM");

    capacity_->setRange(1, 100000);
    capacity_->setValue(50);
    capacity_->setButtonSymbols(QAbstractSpinBox::NoButtons);
    capacity_->setAlignment(Qt::AlignRight);
    name_->setPlaceholderText("Spring Gala");
    venue_->setPlaceholderText("Main Hall");

    auto* title = new QLabel("New Event");
    title->setFont(Theme::titleFont(20));
    titleLabel_ = title;
    auto* lead = new QLabel("Add an event that attendees can book into.");
    lead->setProperty("role", "muted");
    lead->setWordWrap(true);
    leadLabel_ = lead;

    auto* headerGroup = new QVBoxLayout;
    headerGroup->setSpacing(Theme::space1);
    headerGroup->addWidget(title);
    headerGroup->addWidget(lead);

    nameField_ = new FormField("Name", name_);
    venueField_ = new FormField("Venue", venue_);
    auto* dateField = new FormField("Date", date_);
    auto* capacityField = new FormField("Capacity (seats)", capacity_);

    auto* timeContainer = new QWidget;
    auto* timeLayout = new QHBoxLayout(timeContainer);
    timeLayout->setContentsMargins(0, 0, 0, 0);
    timeLayout->setSpacing(Theme::space2);
    auto* colon = new QLabel(":");
    colon->setProperty("role", "muted");
    colon->setAlignment(Qt::AlignCenter);
    timeLayout->addWidget(hour_, 1);
    timeLayout->addWidget(colon);
    timeLayout->addWidget(minute_, 1);
    timeLayout->addWidget(period_, 1);
    auto* timeField = new FormField("Time (Hour : Min · AM / PM)", timeContainer);

    description_->setPlaceholderText("Optional notes or agenda");
    description_->setMaximumHeight(76);
    auto* descField = new FormField("Description", description_);

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
    submitButton_ = create;
    auto* buttons = new QHBoxLayout;
    buttons->setSpacing(Theme::space2);
    buttons->addStretch();
    buttons->addWidget(cancel);
    buttons->addWidget(create);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(Theme::space6, Theme::space6, Theme::space6, Theme::space6);
    layout->setSpacing(Theme::space4);
    layout->addLayout(headerGroup);
    layout->addWidget(nameField_);
    layout->addWidget(venueField_);
    layout->addLayout(row);
    layout->addWidget(timeField);
    layout->addWidget(descField);
    layout->addSpacing(Theme::space2);
    layout->addLayout(buttons);

    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(create, &QPushButton::clicked, this, [this] { if (validate()) accept(); });
    connect(name_, &QLineEdit::textChanged, this, [this] { if (nameField_->hasError()) nameField_->setError(QString()); });
    connect(venue_, &QLineEdit::textChanged, this, [this] { if (venueField_->hasError()) venueField_->setError(QString()); });
}

EventDialog::EventDialog(const QString& name, const QString& date, const QString& time, const QString& venue,
                         int capacity, const QString& description, QWidget* parent)
    : EventDialog(parent) {
    setWindowTitle("Edit Event");
    titleLabel_->setText("Edit Event");
    leadLabel_->setText("Update the event details, schedule, or capacity.");
    submitButton_->setText("Save Changes");
    name_->setText(name);
    const QDate parsed = QDate::fromString(date, "yyyy-MM-dd");
    if (parsed.isValid()) date_->setDate(parsed);
    int h = 9;
    int m = 0;
    bool pm = false;
    if (DateUtil::parseTime(time.toStdString(), h, m, pm)) {
        hour_->setValue(h);
        minute_->setValue(m);
        period_->setCurrentIndex(pm ? 1 : 0);
    }
    venue_->setText(venue);
    capacity_->setValue(capacity);
    description_->setPlainText(description);
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
QString EventDialog::time() const {
    return QString::fromStdString(DateUtil::formatTime12h(hour_->value(), minute_->value(), period_->currentIndex() == 1));
}
QString EventDialog::venue() const { return venue_->text().trimmed(); }
int EventDialog::capacity() const { return capacity_->value(); }
QString EventDialog::description() const { return description_->toPlainText().trimmed(); }
