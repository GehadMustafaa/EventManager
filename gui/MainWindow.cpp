#include "MainWindow.h"
#include <QAction>
#include <QCloseEvent>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QStatusBar>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>
#include <exception>
#include "EventDialog.h"
#include "Storage.h"

namespace {

QTableWidget* makeTable(const QStringList& headers) {
    auto* table = new QTableWidget(0, headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStretchLastSection(true);
    return table;
}

int selectedId(const QTableWidget* table) {
    int row = table->currentRow();
    if (row < 0 || !table->item(row, 0)) return -1;
    return table->item(row, 0)->text().toInt();
}

bool selectById(QTableWidget* table, int id) {
    for (int row = 0; row < table->rowCount(); ++row) {
        if (table->item(row, 0)->text().toInt() == id) {
            table->selectRow(row);
            return true;
        }
    }
    return false;
}

void setRow(QTableWidget* table, int row, const QStringList& cells) {
    for (int col = 0; col < cells.size(); ++col) table->setItem(row, col, new QTableWidgetItem(cells[col]));
}

QString toQ(const std::string& s) {
    return QString::fromStdString(s);
}

}

MainWindow::MainWindow(const QString& dataPath, QWidget* parent) : QMainWindow(parent), dataPath_(dataPath) {
    buildUi();
    if (QFile::exists(dataPath_)) {
        try {
            Storage::load(manager_, dataPath_.toStdString());
        } catch (const std::exception& ex) {
            showError(ex.what());
        }
    }
    refreshAll();
    updateTitle();
}

void MainWindow::buildUi() {
    resize(1100, 720);

    auto* fileMenu = menuBar()->addMenu("&File");
    auto* saveAction = fileMenu->addAction("&Save", QKeySequence::Save, this, &MainWindow::save);
    auto* loadAction = fileMenu->addAction("&Load", this, &MainWindow::load);
    fileMenu->addSeparator();
    auto* exitAction = fileMenu->addAction("E&xit", this, &QWidget::close);
    Q_UNUSED(saveAction);
    Q_UNUSED(loadAction);
    Q_UNUSED(exitAction);

    eventSearch_ = new QLineEdit;
    eventSearch_->setPlaceholderText("Find event by exact name, then press Enter");
    eventsTable_ = makeTable({"ID", "Name", "Date", "Venue", "Seats", "Waitlist"});
    auto* addEventButton = new QPushButton("Add Event");
    auto* deleteEventButton = new QPushButton("Delete Event");
    auto* eventButtons = new QHBoxLayout;
    eventButtons->addWidget(addEventButton);
    eventButtons->addWidget(deleteEventButton);
    eventButtons->addStretch();
    auto* eventsBox = new QGroupBox("Events");
    auto* eventsLayout = new QVBoxLayout(eventsBox);
    eventsLayout->addWidget(eventSearch_);
    eventsLayout->addWidget(eventsTable_);
    eventsLayout->addLayout(eventButtons);

    attendeeSearch_ = new QLineEdit;
    attendeeSearch_->setPlaceholderText("Find attendee by exact email, then press Enter");
    attendeesTable_ = makeTable({"ID", "Name", "Email"});
    auto* registerButton = new QPushButton("Register Attendee");
    auto* attendeeButtons = new QHBoxLayout;
    attendeeButtons->addWidget(registerButton);
    attendeeButtons->addStretch();
    auto* attendeesBox = new QGroupBox("Attendees");
    auto* attendeesLayout = new QVBoxLayout(attendeesBox);
    attendeesLayout->addWidget(attendeeSearch_);
    attendeesLayout->addWidget(attendeesTable_);
    attendeesLayout->addLayout(attendeeButtons);

    auto* topRow = new QHBoxLayout;
    topRow->addWidget(eventsBox, 3);
    topRow->addWidget(attendeesBox, 2);

    auto* bookButton = new QPushButton("Book Selected Attendee Into Selected Event");
    auto* cancelButton = new QPushButton("Cancel Selected Attendee's Booking");
    auto* actionRow = new QHBoxLayout;
    actionRow->addWidget(bookButton);
    actionRow->addWidget(cancelButton);

    detailsTitle_ = new QLabel;
    confirmedList_ = new QListWidget;
    waitlistList_ = new QListWidget;
    auto* confirmedBox = new QGroupBox("Confirmed");
    auto* confirmedLayout = new QVBoxLayout(confirmedBox);
    confirmedLayout->addWidget(confirmedList_);
    auto* waitlistBox = new QGroupBox("Waiting list (first in line at top)");
    auto* waitlistLayout = new QVBoxLayout(waitlistBox);
    waitlistLayout->addWidget(waitlistList_);
    auto* listsRow = new QHBoxLayout;
    listsRow->addWidget(confirmedBox);
    listsRow->addWidget(waitlistBox);
    auto* detailsBox = new QGroupBox("Selected event");
    auto* detailsLayout = new QVBoxLayout(detailsBox);
    detailsLayout->addWidget(detailsTitle_);
    detailsLayout->addLayout(listsRow);

    auto* central = new QWidget;
    auto* root = new QVBoxLayout(central);
    root->addLayout(topRow, 3);
    root->addLayout(actionRow);
    root->addWidget(detailsBox, 2);
    setCentralWidget(central);

    connect(addEventButton, &QPushButton::clicked, this, &MainWindow::addEvent);
    connect(deleteEventButton, &QPushButton::clicked, this, &MainWindow::deleteEvent);
    connect(registerButton, &QPushButton::clicked, this, &MainWindow::registerAttendee);
    connect(bookButton, &QPushButton::clicked, this, &MainWindow::bookSelected);
    connect(cancelButton, &QPushButton::clicked, this, &MainWindow::cancelSelected);
    connect(eventSearch_, &QLineEdit::returnPressed, this, &MainWindow::searchEvent);
    connect(attendeeSearch_, &QLineEdit::returnPressed, this, &MainWindow::searchAttendee);
    connect(eventsTable_, &QTableWidget::itemSelectionChanged, this, &MainWindow::refreshDetails);
}

void MainWindow::refreshAll() {
    int eventId = selectedId(eventsTable_);
    int attendeeId = selectedId(attendeesTable_);

    eventsTable_->blockSignals(true);
    eventsTable_->setRowCount(0);
    for (const Event* e : manager_.events()) {
        int row = eventsTable_->rowCount();
        eventsTable_->insertRow(row);
        setRow(eventsTable_, row,
               {QString::number(e->id()), toQ(e->name()), toQ(e->date()), toQ(e->venue()),
                QString("%1 / %2").arg(e->seatsAvailable()).arg(e->capacity()),
                QString::number(e->waitlistCount())});
    }
    eventsTable_->resizeColumnsToContents();
    eventsTable_->horizontalHeader()->setStretchLastSection(true);
    if ((eventId < 0 || !selectById(eventsTable_, eventId)) && eventsTable_->rowCount() > 0) eventsTable_->selectRow(0);
    eventsTable_->blockSignals(false);

    attendeesTable_->setRowCount(0);
    for (const Attendee& a : manager_.attendees()) {
        int row = attendeesTable_->rowCount();
        attendeesTable_->insertRow(row);
        setRow(attendeesTable_, row, {QString::number(a.id), toQ(a.name), toQ(a.email)});
    }
    attendeesTable_->resizeColumnsToContents();
    attendeesTable_->horizontalHeader()->setStretchLastSection(true);
    if (attendeeId >= 0) selectById(attendeesTable_, attendeeId);

    refreshDetails();
}

void MainWindow::refreshDetails() {
    confirmedList_->clear();
    waitlistList_->clear();

    const Event* e = manager_.findEvent(selectedId(eventsTable_));
    if (!e) {
        detailsTitle_->setText("No event selected");
        return;
    }

    detailsTitle_->setText(QString("%1  |  %2  |  %3  |  seats available: %4 of %5")
                               .arg(toQ(e->name()), toQ(e->date()), toQ(e->venue()))
                               .arg(e->seatsAvailable())
                               .arg(e->capacity()));

    std::vector<int> confirmed = e->confirmedIds();
    std::sort(confirmed.begin(), confirmed.end());
    for (int id : confirmed) confirmedList_->addItem(attendeeLabel(id));

    int position = 1;
    for (int id : e->waitlistIds()) waitlistList_->addItem(QString("%1. %2").arg(position++).arg(attendeeLabel(id)));
}

QString MainWindow::attendeeLabel(int attendeeId) const {
    const Attendee* a = manager_.findAttendee(attendeeId);
    if (!a) return QString("#%1").arg(attendeeId);
    return QString("#%1  %2  <%3>").arg(a->id).arg(toQ(a->name), toQ(a->email));
}

void MainWindow::addEvent() {
    EventDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) return;
    try {
        int id = manager_.createEvent(dialog.name().toStdString(), dialog.date().toStdString(),
                                      dialog.venue().toStdString(), dialog.capacity());
        markDirty();
        refreshAll();
        selectById(eventsTable_, id);
        statusBar()->showMessage(QString("Created event #%1").arg(id), 4000);
    } catch (const std::exception& ex) {
        showError(ex.what());
    }
}

void MainWindow::deleteEvent() {
    int id = selectedId(eventsTable_);
    if (id < 0) {
        statusBar()->showMessage("Select an event first", 4000);
        return;
    }
    if (QMessageBox::question(this, "Delete event", QString("Delete event #%1 and all its bookings?").arg(id)) != QMessageBox::Yes)
        return;
    manager_.deleteEvent(id);
    markDirty();
    refreshAll();
    statusBar()->showMessage(QString("Deleted event #%1").arg(id), 4000);
}

void MainWindow::registerAttendee() {
    bool ok = false;
    QString name = QInputDialog::getText(this, "Register attendee", "Name:", QLineEdit::Normal, "", &ok).trimmed();
    if (!ok) return;
    QString email = QInputDialog::getText(this, "Register attendee", "Email:", QLineEdit::Normal, "", &ok).trimmed();
    if (!ok) return;
    try {
        int id = manager_.registerAttendee(name.toStdString(), email.toStdString());
        markDirty();
        refreshAll();
        selectById(attendeesTable_, id);
        statusBar()->showMessage(QString("Registered attendee #%1").arg(id), 4000);
    } catch (const std::exception& ex) {
        showError(ex.what());
    }
}

void MainWindow::bookSelected() {
    int eventId = selectedId(eventsTable_);
    int attendeeId = selectedId(attendeesTable_);
    if (eventId < 0 || attendeeId < 0) {
        statusBar()->showMessage("Select an event and an attendee first", 4000);
        return;
    }
    try {
        QString message;
        switch (manager_.book(eventId, attendeeId)) {
            case Event::BookResult::Confirmed: message = "Booking confirmed"; break;
            case Event::BookResult::Waitlisted: message = "Event is full: attendee added to the waiting list"; break;
            case Event::BookResult::Duplicate: message = "Attendee is already booked or waitlisted for this event"; break;
        }
        markDirty();
        refreshAll();
        statusBar()->showMessage(message, 5000);
    } catch (const std::exception& ex) {
        showError(ex.what());
    }
}

void MainWindow::cancelSelected() {
    int eventId = selectedId(eventsTable_);
    int attendeeId = selectedId(attendeesTable_);
    if (eventId < 0 || attendeeId < 0) {
        statusBar()->showMessage("Select an event and an attendee first", 4000);
        return;
    }
    try {
        Event::CancelResult result = manager_.cancel(eventId, attendeeId);
        if (!result.cancelled) {
            statusBar()->showMessage("This attendee has no booking for the selected event", 5000);
            return;
        }
        QString message = "Booking cancelled";
        if (result.promotedAttendeeId)
            message += QString(". Promoted from waiting list: %1").arg(attendeeLabel(*result.promotedAttendeeId));
        markDirty();
        refreshAll();
        statusBar()->showMessage(message, 6000);
    } catch (const std::exception& ex) {
        showError(ex.what());
    }
}

void MainWindow::searchEvent() {
    const Event* e = manager_.findEventByName(eventSearch_->text().trimmed().toStdString());
    if (e && selectById(eventsTable_, e->id())) statusBar()->showMessage(QString("Found event #%1").arg(e->id()), 3000);
    else statusBar()->showMessage("Event not found", 3000);
}

void MainWindow::searchAttendee() {
    const Attendee* a = manager_.findAttendeeByEmail(attendeeSearch_->text().trimmed().toStdString());
    if (a && selectById(attendeesTable_, a->id)) statusBar()->showMessage(QString("Found attendee #%1").arg(a->id), 3000);
    else statusBar()->showMessage("Attendee not found", 3000);
}

bool MainWindow::save() {
    try {
        QDir().mkpath(QFileInfo(dataPath_).absolutePath());
        Storage::save(manager_, dataPath_.toStdString());
        dirty_ = false;
        updateTitle();
        statusBar()->showMessage("Saved to " + dataPath_, 4000);
        return true;
    } catch (const std::exception& ex) {
        showError(ex.what());
        return false;
    }
}

void MainWindow::load() {
    if (dirty_ && QMessageBox::question(this, "Load", "Discard unsaved changes and reload from file?") != QMessageBox::Yes)
        return;
    try {
        Storage::load(manager_, dataPath_.toStdString());
        dirty_ = false;
        refreshAll();
        updateTitle();
        statusBar()->showMessage("Loaded from " + dataPath_, 4000);
    } catch (const std::exception& ex) {
        refreshAll();
        showError(ex.what());
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (!dirty_) {
        event->accept();
        return;
    }
    auto answer = QMessageBox::question(this, "Unsaved changes", "Save changes before closing?",
                                        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (answer == QMessageBox::Cancel || (answer == QMessageBox::Save && !save())) event->ignore();
    else event->accept();
}

void MainWindow::markDirty() {
    dirty_ = true;
    updateTitle();
}

void MainWindow::updateTitle() {
    setWindowTitle(QString("Smart Event Management%1").arg(dirty_ ? " *" : ""));
}

void MainWindow::showError(const QString& message) {
    QMessageBox::warning(this, "Error", message);
}
