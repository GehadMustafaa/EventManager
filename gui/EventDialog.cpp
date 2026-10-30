#include "EventDialog.h"
#include <QDate>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QVBoxLayout>

EventDialog::EventDialog(QWidget* parent)
    : QDialog(parent),
      name_(new QLineEdit),
      date_(new QDateEdit(QDate::currentDate())),
      venue_(new QLineEdit),
      capacity_(new QSpinBox) {
    setWindowTitle("New Event");
    setMinimumWidth(360);

    date_->setCalendarPopup(true);
    date_->setDisplayFormat("yyyy-MM-dd");
    capacity_->setRange(1, 100000);
    capacity_->setValue(50);

    auto* form = new QFormLayout;
    form->addRow("Name", name_);
    form->addRow("Date", date_);
    form->addRow("Venue", venue_);
    form->addRow("Capacity", capacity_);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

QString EventDialog::name() const { return name_->text().trimmed(); }
QString EventDialog::date() const { return date_->date().toString("yyyy-MM-dd"); }
QString EventDialog::venue() const { return venue_->text().trimmed(); }
int EventDialog::capacity() const { return capacity_->value(); }
