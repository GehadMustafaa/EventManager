#include "MainWindow.h"
#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QKeySequence>
#include <QMenu>
#include <QCloseEvent>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>
#include <exception>
#include "AttendeeDialog.h"
#include "Delegates.h"
#include "EventDialog.h"
#include "Storage.h"
#include "Theme.h"

namespace {

QString toQ(const std::string& s) { return QString::fromStdString(s); }

QLabel* makeLabel(const QString& text, const char* role) {
    auto* label = new QLabel(text);
    label->setProperty("role", role);
    return label;
}

QPushButton* makeButton(const QString& text, const char* variant = nullptr) {
    auto* button = new QPushButton(text);
    if (variant) button->setProperty("variant", variant);
    // Focus ring appears for keyboard users only; mouse clicks do not leave a ring behind.
    button->setFocusPolicy(Qt::TabFocus);
    return button;
}

QFrame* makePanel() {
    auto* panel = new QFrame;
    panel->setObjectName("panel");
    return panel;
}

void setChip(QLabel* chip, const char* kind, const QString& text) {
    chip->setProperty("chip", kind);
    chip->setText(text);
    Theme::repolish(chip);
}

// Table: no grid, no frame, 36px rows, painted by TableDelegate.
QTableWidget* makeTable(const QStringList& headers) {
    auto* table = new QTableWidget(0, headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setShowGrid(false);
    table->setFrameShape(QFrame::NoFrame);
    table->setWordWrap(false);
    table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    table->verticalHeader()->setVisible(false);
    table->verticalHeader()->setDefaultSectionSize(Theme::rowHeight);
    table->horizontalHeader()->setHighlightSections(false);
    table->horizontalHeader()->setSectionsClickable(false);
    table->horizontalHeader()->setStretchLastSection(false);
    return table;
}

void setHeaderAlignment(QTableWidget* table, int column, Qt::Alignment alignment) {
    if (auto* item = table->horizontalHeaderItem(column)) item->setTextAlignment(alignment | Qt::AlignVCenter);
}

QTableWidgetItem* cell(const QString& text, bool secondary = false, bool right = false) {
    auto* item = new QTableWidgetItem(text);
    item->setData(ItemRole::Tone, secondary ? 1 : 0);
    item->setData(ItemRole::AlignRight, right);
    return item;
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

// Empty states say what is missing and point at the first action.
QWidget* makeEmptyState(const QString& title, const QString& body, QPushButton* action = nullptr) {
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(Theme::space6, Theme::space6, Theme::space6, Theme::space6);
    layout->setSpacing(Theme::space2);
    layout->addStretch();
    auto* heading = makeLabel(title, "emptyTitle");
    heading->setAlignment(Qt::AlignHCenter);
    auto* text = makeLabel(body, "muted");
    text->setAlignment(Qt::AlignHCenter);
    text->setWordWrap(true);
    layout->addWidget(heading);
    layout->addWidget(text);
    if (action) {
        layout->addSpacing(Theme::space2);
        layout->addWidget(action, 0, Qt::AlignHCenter);
    }
    layout->addStretch();
    return page;
}

// Panel header: title, count, then right-aligned actions.
QHBoxLayout* makeHeader(const QString& title, QLabel* count, const QList<QPushButton*>& actions) {
    auto* row = new QHBoxLayout;
    row->setSpacing(Theme::space2);
    row->addWidget(makeLabel(title, "panelTitle"));
    row->addWidget(count);
    row->addStretch();
    for (QPushButton* action : actions) row->addWidget(action);
    return row;
}

// The core reports problems as exception text; say what happened and what to do next.
QString friendlyError(const std::exception& ex) {
    const QString raw = QString::fromUtf8(ex.what());
    if (raw == "event name already exists") return "An event with that name already exists. Choose a different name.";
    if (raw == "email already registered") return "That email is already registered. Use a different email, or search for the attendee.";
    if (raw.endsWith(" must not be empty")) {
        QString field = raw.left(raw.size() - 18);
        return field.left(1).toUpper() + field.mid(1) + " is required. Fill it in and try again.";
    }
    if (raw.endsWith(" contains an invalid character")) return "Remove any | characters and line breaks, then try again.";
    return raw;
}

}  // namespace

MainWindow::MainWindow(const QString& dataPath, QWidget* parent) : QMainWindow(parent), dataPath_(dataPath) {
    buildUi();
    if (QFile::exists(dataPath_)) {
        try {
            Storage::load(manager_, dataPath_.toStdString());
        } catch (const std::exception& ex) {
            showError("Couldn't Open Data File", QString("%1\n\nThe app started with an empty list. Fix or remove %2, then use File > Reload From File.")
                                                     .arg(friendlyError(ex), dataPath_));
        }
    }
    refreshAll();
    updateTitle();
}

void MainWindow::buildUi() {
    resize(1180, 760);
    setMinimumSize(980, 600);

    buildMenus();

    auto* columns = new QSplitter(Qt::Vertical);
    columns->setChildrenCollapsible(false);
    columns->setHandleWidth(Theme::space3);
    columns->addWidget(buildEventsPanel());
    columns->addWidget(buildAttendeesPanel());
    columns->setStretchFactor(0, 3);
    columns->setStretchFactor(1, 2);

    auto* root = new QSplitter(Qt::Horizontal);
    root->setChildrenCollapsible(false);
    root->setHandleWidth(Theme::space3);
    root->addWidget(columns);
    root->addWidget(buildInspector());
    root->setStretchFactor(0, 3);
    root->setStretchFactor(1, 2);
    root->setSizes({680, 440});

    auto* central = new QWidget;
    central->setObjectName("central");
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(Theme::space3, Theme::space2, Theme::space3, Theme::space3);
    layout->addWidget(root);
    setCentralWidget(central);

    dirtyChip_ = new QLabel;
    setChip(dirtyChip_, "warning", "Unsaved changes");
    dirtyChip_->hide();
    statusBar()->setSizeGripEnabled(false);
    statusBar()->addPermanentWidget(dirtyChip_);

    connect(eventsTable_, &QTableWidget::itemSelectionChanged, this, &MainWindow::refreshDetails);
    connect(attendeesTable_, &QTableWidget::itemSelectionChanged, this, &MainWindow::updateActions);
}

void MainWindow::buildMenus() {
    auto addAction = [this](QMenu* menu, const QString& text, const QKeySequence& shortcut, void (MainWindow::*slot)()) {
        QAction* action = menu->addAction(text);
        if (!shortcut.isEmpty()) action->setShortcut(shortcut);
        connect(action, &QAction::triggered, this, slot);
    };

    auto* fileMenu = menuBar()->addMenu("&File");
    addAction(fileMenu, "New &Event", QKeySequence::New, &MainWindow::addEvent);
    addAction(fileMenu, "&Register Attendee", QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N), &MainWindow::registerAttendee);
    fileMenu->addSeparator();
    QAction* saveAction = fileMenu->addAction("&Save");
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, [this] { save(); });
    addAction(fileMenu, "Re&load From File", QKeySequence(), &MainWindow::load);
    fileMenu->addSeparator();
    QAction* quitAction = fileMenu->addAction("&Quit");
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, [this] { close(); });

    auto* viewMenu = menuBar()->addMenu("&View");
    darkModeAction_ = viewMenu->addAction("&Dark Mode");
    darkModeAction_->setCheckable(true);
    darkModeAction_->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D));
    darkModeAction_->setChecked(Theme::mode() == Theme::Mode::Dark);
    connect(darkModeAction_, &QAction::toggled, this, &MainWindow::setDarkMode);
}

QWidget* MainWindow::buildEventsPanel() {
    eventsCount_ = makeLabel("", "caption");
    deleteEventButton_ = makeButton("Delete Event", "danger");
    auto* addButton = makeButton("New Event");
    connect(addButton, &QPushButton::clicked, this, &MainWindow::addEvent);
    connect(deleteEventButton_, &QPushButton::clicked, this, &MainWindow::deleteEvent);

    eventSearch_ = new QLineEdit;
    eventSearch_->setPlaceholderText("Search by exact event name, then press Enter");
    eventSearch_->setClearButtonEnabled(true);
    connect(eventSearch_, &QLineEdit::returnPressed, this, &MainWindow::searchEvent);

    eventsTable_ = makeTable({"ID", "Name", "Date", "Venue", "Booked", "Waitlist"});
    auto* header = eventsTable_->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Fixed);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::Fixed);
    header->setSectionResizeMode(3, QHeaderView::Stretch);
    header->setSectionResizeMode(4, QHeaderView::Fixed);
    header->setSectionResizeMode(5, QHeaderView::Fixed);
    eventsTable_->setColumnWidth(0, 56);
    eventsTable_->setColumnWidth(2, 112);
    eventsTable_->setColumnWidth(4, 150);
    eventsTable_->setColumnWidth(5, 84);
    setHeaderAlignment(eventsTable_, 5, Qt::AlignRight);
    trackHover(eventsTable_, new TableDelegate(eventsTable_));

    auto* emptyAdd = makeButton("New Event");
    connect(emptyAdd, &QPushButton::clicked, this, &MainWindow::addEvent);
    eventsStack_ = new QStackedWidget;
    eventsStack_->addWidget(makeEmptyState("No events yet", "Create your first event, then book attendees into it.", emptyAdd));
    eventsStack_->addWidget(eventsTable_);

    auto* panel = makePanel();
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(Theme::space4, Theme::space3, Theme::space4, Theme::space3);
    layout->setSpacing(Theme::space3);
    layout->addLayout(makeHeader("Events", eventsCount_, {deleteEventButton_, addButton}));
    layout->addWidget(eventSearch_);
    layout->addWidget(eventsStack_, 1);
    return panel;
}

QWidget* MainWindow::buildAttendeesPanel() {
    attendeesCount_ = makeLabel("", "caption");
    auto* registerButton = makeButton("Register Attendee");
    connect(registerButton, &QPushButton::clicked, this, &MainWindow::registerAttendee);

    attendeeSearch_ = new QLineEdit;
    attendeeSearch_->setPlaceholderText("Search by exact email, then press Enter");
    attendeeSearch_->setClearButtonEnabled(true);
    connect(attendeeSearch_, &QLineEdit::returnPressed, this, &MainWindow::searchAttendee);

    attendeesTable_ = makeTable({"ID", "Name", "Email"});
    auto* header = attendeesTable_->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Fixed);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setSectionResizeMode(2, QHeaderView::Stretch);
    attendeesTable_->setColumnWidth(0, 56);
    trackHover(attendeesTable_, new TableDelegate(attendeesTable_));

    auto* emptyAdd = makeButton("Register Attendee");
    connect(emptyAdd, &QPushButton::clicked, this, &MainWindow::registerAttendee);
    attendeesStack_ = new QStackedWidget;
    attendeesStack_->addWidget(makeEmptyState("No attendees yet", "Register someone to start taking bookings.", emptyAdd));
    attendeesStack_->addWidget(attendeesTable_);

    auto* panel = makePanel();
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(Theme::space4, Theme::space3, Theme::space4, Theme::space3);
    layout->setSpacing(Theme::space3);
    layout->addLayout(makeHeader("Attendees", attendeesCount_, {registerButton}));
    layout->addWidget(attendeeSearch_);
    layout->addWidget(attendeesStack_, 1);
    return panel;
}

QWidget* MainWindow::buildInspector() {
    // Page 0: nothing selected.
    auto* none = makeEmptyState("No event selected", "Select an event to see who is confirmed and who is waiting.");

    // Page 1: the selected event.
    eventTitle_ = new QLabel;
    eventTitle_->setFont(Theme::titleFont(20));
    eventTitle_->setWordWrap(true);
    eventMeta_ = makeLabel("", "muted");
    eventMeta_->setWordWrap(true);

    seatsLabel_ = new QLabel;
    statusChip_ = new QLabel;
    setChip(statusChip_, "success", "Open");
    seatMeter_ = new QProgressBar;
    seatMeter_->setTextVisible(false);
    auto* seatRow = new QHBoxLayout;
    seatRow->addWidget(seatsLabel_);
    seatRow->addStretch();
    seatRow->addWidget(statusChip_);

    // Booking actions live next to the event they change.
    actionName_ = new QLabel;
    actionHint_ = makeLabel("", "caption");
    actionHint_->setWordWrap(true);
    bookButton_ = makeButton("Book Attendee", "primary");
    cancelButton_ = makeButton("Cancel Booking");
    connect(bookButton_, &QPushButton::clicked, this, &MainWindow::bookSelected);
    connect(cancelButton_, &QPushButton::clicked, this, &MainWindow::cancelSelected);
    auto* actionButtons = new QHBoxLayout;
    actionButtons->setSpacing(Theme::space2);
    actionButtons->addWidget(bookButton_);
    actionButtons->addWidget(cancelButton_);
    actionButtons->addStretch();

    auto* well = new QFrame;
    well->setObjectName("well");
    auto* wellLayout = new QVBoxLayout(well);
    wellLayout->setContentsMargins(Theme::space3, Theme::space3, Theme::space3, Theme::space3);
    wellLayout->setSpacing(Theme::space1);
    wellLayout->addWidget(makeLabel("Selected attendee", "caption"));
    wellLayout->addWidget(actionName_);
    wellLayout->addWidget(actionHint_);
    wellLayout->addSpacing(Theme::space2);
    wellLayout->addLayout(actionButtons);

    confirmedCount_ = makeLabel("", "caption");
    confirmedList_ = new QListWidget;
    confirmedList_->setSelectionMode(QAbstractItemView::NoSelection);
    confirmedList_->setFrameShape(QFrame::NoFrame);
    trackHover(confirmedList_, new PersonDelegate(confirmedList_));
    confirmedStack_ = new QStackedWidget;
    confirmedStack_->addWidget(makeEmptyState("No confirmed attendees", "Book an attendee to fill a seat."));
    confirmedStack_->addWidget(confirmedList_);

    waitlistCount_ = makeLabel("", "caption");
    waitlistList_ = new QListWidget;
    waitlistList_->setSelectionMode(QAbstractItemView::NoSelection);
    waitlistList_->setFrameShape(QFrame::NoFrame);
    trackHover(waitlistList_, new PersonDelegate(waitlistList_));
    waitlistStack_ = new QStackedWidget;
    waitlistStack_->addWidget(makeEmptyState("Waitlist is empty", "People who book a full event wait here, first in line at the top."));
    waitlistStack_->addWidget(waitlistList_);

    auto* confirmedHeader = new QHBoxLayout;
    confirmedHeader->setSpacing(Theme::space2);
    confirmedHeader->addWidget(makeLabel("Confirmed", "panelTitle"));
    confirmedHeader->addWidget(confirmedCount_);
    confirmedHeader->addStretch();
    auto* waitlistHeader = new QHBoxLayout;
    waitlistHeader->setSpacing(Theme::space2);
    waitlistHeader->addWidget(makeLabel("Waitlist", "panelTitle"));
    waitlistHeader->addWidget(waitlistCount_);
    waitlistHeader->addStretch();

    auto* detail = new QWidget;
    auto* layout = new QVBoxLayout(detail);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(Theme::space3);
    layout->addWidget(eventTitle_);
    layout->addWidget(eventMeta_);
    layout->addSpacing(Theme::space1);
    layout->addLayout(seatRow);
    layout->addWidget(seatMeter_);
    layout->addSpacing(Theme::space1);
    layout->addWidget(well);
    layout->addLayout(confirmedHeader);
    layout->addWidget(confirmedStack_, 1);
    layout->addLayout(waitlistHeader);
    layout->addWidget(waitlistStack_, 1);

    inspectorStack_ = new QStackedWidget;
    inspectorStack_->addWidget(none);
    inspectorStack_->addWidget(detail);

    auto* panel = makePanel();
    auto* outer = new QVBoxLayout(panel);
    outer->setContentsMargins(Theme::space4, Theme::space4, Theme::space4, Theme::space4);
    outer->addWidget(inspectorStack_);
    panel->setMinimumWidth(380);
    return panel;
}

void MainWindow::trackHover(QAbstractItemView* view, HoverDelegate* delegate) {
    view->setItemDelegate(delegate);
    view->setMouseTracking(true);
    view->viewport()->installEventFilter(this);
    hoverViewports_.insert(view->viewport(), delegate);
    connect(view, &QAbstractItemView::entered, this, [view, delegate](const QModelIndex& index) {
        delegate->hoverRow = index.row();
        view->viewport()->update();
    });
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::Leave) {
        auto it = hoverViewports_.constFind(watched);
        if (it != hoverViewports_.constEnd()) {
            it.value()->hoverRow = -1;
            static_cast<QWidget*>(watched)->update();
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::refreshAll() {
    const int eventId = selectedId(eventsTable_);
    const int attendeeId = selectedId(attendeesTable_);

    eventsTable_->blockSignals(true);
    eventsTable_->setRowCount(0);
    const auto events = manager_.events();
    for (const Event* e : events) {
        const int row = eventsTable_->rowCount();
        eventsTable_->insertRow(row);
        auto* booked = cell(QString("%1 / %2").arg(int(e->confirmedCount())).arg(e->capacity()), true);
        booked->setData(ItemRole::Kind, ItemRole::Seats);
        booked->setData(ItemRole::Booked, int(e->confirmedCount()));
        booked->setData(ItemRole::Capacity, e->capacity());
        eventsTable_->setItem(row, 0, cell(QString::number(e->id()), true));
        eventsTable_->setItem(row, 1, cell(toQ(e->name())));
        eventsTable_->setItem(row, 2, cell(toQ(e->date()), true));
        eventsTable_->setItem(row, 3, cell(toQ(e->venue()), true));
        eventsTable_->setItem(row, 4, booked);
        eventsTable_->setItem(row, 5, cell(QString::number(e->waitlistCount()), e->waitlistCount() == 0, true));
    }
    if ((eventId < 0 || !selectById(eventsTable_, eventId)) && eventsTable_->rowCount() > 0) eventsTable_->selectRow(0);
    eventsTable_->blockSignals(false);
    eventsStack_->setCurrentIndex(eventsTable_->rowCount() > 0 ? 1 : 0);
    eventsCount_->setText(QString::number(eventsTable_->rowCount()));
    eventSearch_->setEnabled(eventsTable_->rowCount() > 0);
    deleteEventButton_->setEnabled(eventsTable_->rowCount() > 0);

    attendeesTable_->blockSignals(true);
    attendeesTable_->setRowCount(0);
    for (const Attendee& a : manager_.attendees()) {
        const int row = attendeesTable_->rowCount();
        attendeesTable_->insertRow(row);
        attendeesTable_->setItem(row, 0, cell(QString::number(a.id), true));
        attendeesTable_->setItem(row, 1, cell(toQ(a.name)));
        attendeesTable_->setItem(row, 2, cell(toQ(a.email), true));
    }
    if (attendeeId >= 0) selectById(attendeesTable_, attendeeId);
    attendeesTable_->blockSignals(false);
    attendeesStack_->setCurrentIndex(attendeesTable_->rowCount() > 0 ? 1 : 0);
    attendeesCount_->setText(QString::number(attendeesTable_->rowCount()));
    attendeeSearch_->setEnabled(attendeesTable_->rowCount() > 0);

    refreshDetails();
}

void MainWindow::refreshDetails() {
    confirmedList_->clear();
    waitlistList_->clear();

    const Event* e = manager_.findEvent(selectedId(eventsTable_));
    if (!e) {
        inspectorStack_->setCurrentIndex(0);
        updateActions();
        return;
    }
    inspectorStack_->setCurrentIndex(1);

    eventTitle_->setText(toQ(e->name()));
    const QDate date = QDate::fromString(toQ(e->date()), "yyyy-MM-dd");
    const QString dateText = date.isValid() ? QLocale().toString(date, "ddd d MMM yyyy") : toQ(e->date());
    eventMeta_->setText(QString("%1  \xC2\xB7  %2").arg(dateText, toQ(e->venue())));

    const int capacity = e->capacity();
    const int booked = int(e->confirmedCount());
    const bool full = booked >= capacity;
    const bool nearlyFull = !full && booked * 10 >= capacity * 8;
    seatsLabel_->setText(QString("%1 of %2 seats booked").arg(booked).arg(capacity));
    seatMeter_->setRange(0, capacity);
    seatMeter_->setValue(booked);
    seatMeter_->setProperty("state", full ? "full" : nearlyFull ? "warn" : "ok");
    Theme::repolish(seatMeter_);
    if (full) setChip(statusChip_, "danger", "Full");
    else if (nearlyFull) setChip(statusChip_, "warning", "Almost Full");
    else setChip(statusChip_, "success", "Open");

    std::vector<int> confirmed = e->confirmedIds();
    std::sort(confirmed.begin(), confirmed.end());
    for (int id : confirmed) {
        const Attendee* a = manager_.findAttendee(id);
        auto* item = new QListWidgetItem(a ? toQ(a->name) : QString("Attendee #%1").arg(id));
        item->setData(ItemRole::Email, a ? toQ(a->email) : QString());
        confirmedList_->addItem(item);
    }
    int position = 1;
    for (int id : e->waitlistIds()) {
        const Attendee* a = manager_.findAttendee(id);
        auto* item = new QListWidgetItem(a ? toQ(a->name) : QString("Attendee #%1").arg(id));
        item->setData(ItemRole::Email, a ? toQ(a->email) : QString());
        item->setData(ItemRole::Position, position++);
        waitlistList_->addItem(item);
    }
    confirmedCount_->setText(QString::number(confirmedList_->count()));
    waitlistCount_->setText(QString::number(waitlistList_->count()));
    confirmedStack_->setCurrentIndex(confirmedList_->count() > 0 ? 1 : 0);
    waitlistStack_->setCurrentIndex(waitlistList_->count() > 0 ? 1 : 0);

    updateActions();
}

void MainWindow::updateActions() {
    const Event* e = manager_.findEvent(selectedId(eventsTable_));
    const Attendee* a = manager_.findAttendee(selectedAttendeeId());

    if (!a) {
        actionName_->setText("None");
        actionHint_->setText("Select an attendee in the list to book them into this event.");
        bookButton_->setEnabled(false);
        cancelButton_->setEnabled(false);
        return;
    }
    actionName_->setText(QString("%1  (%2)").arg(toQ(a->name), toQ(a->email)));
    if (!e) {
        actionHint_->setText("Select an event to book this attendee.");
        bookButton_->setEnabled(false);
        cancelButton_->setEnabled(false);
        return;
    }

    const bool confirmed = e->isConfirmed(a->id);
    const bool waitlisted = e->isWaitlisted(a->id);
    if (confirmed) {
        actionHint_->setText("Confirmed for this event.");
    } else if (waitlisted) {
        int position = 1;
        for (int id : e->waitlistIds()) {
            if (id == a->id) break;
            ++position;
        }
        actionHint_->setText(QString("On the waitlist, position %1.").arg(position));
    } else if (e->seatsAvailable() <= 0) {
        actionHint_->setText("Not booked. This event is full, so booking adds them to the waitlist.");
    } else {
        actionHint_->setText("Not booked for this event yet.");
    }
    bookButton_->setEnabled(!confirmed && !waitlisted);
    cancelButton_->setEnabled(confirmed || waitlisted);
}

QString MainWindow::attendeeName(int attendeeId) const {
    const Attendee* a = manager_.findAttendee(attendeeId);
    return a ? toQ(a->name) : QString("Attendee #%1").arg(attendeeId);
}

int MainWindow::selectedAttendeeId() const {
    return selectedId(attendeesTable_);
}

void MainWindow::addEvent() {
    EventDialog dialog(this);
    while (dialog.exec() == QDialog::Accepted) {
        try {
            const int id = manager_.createEvent(dialog.name().toStdString(), dialog.date().toStdString(),
                                                dialog.venue().toStdString(), dialog.capacity());
            markDirty();
            refreshAll();
            selectById(eventsTable_, id);
            statusBar()->showMessage(QString("Event %1 created").arg(Theme::quoted(dialog.name())), 5000);
            return;
        } catch (const std::exception& ex) {
            dialog.showNameError(friendlyError(ex));
        }
    }
}

void MainWindow::deleteEvent() {
    const Event* e = manager_.findEvent(selectedId(eventsTable_));
    if (!e) {
        statusBar()->showMessage("Select an event to delete", 4000);
        return;
    }
    const int id = e->id();
    const QString name = toQ(e->name());
    const int bookings = int(e->confirmedCount());
    const int waiting = int(e->waitlistCount());

    QMessageBox box(QMessageBox::NoIcon, "Delete Event?",
                    QString("Delete %1?").arg(Theme::quoted(name)), QMessageBox::NoButton, this);
    box.setInformativeText(QString("This removes %1 confirmed %2 and %3 waitlist %4. You can't undo this.")
                               .arg(bookings).arg(bookings == 1 ? "booking" : "bookings")
                               .arg(waiting).arg(waiting == 1 ? "entry" : "entries"));
    QPushButton* confirm = box.addButton("Delete Event", QMessageBox::DestructiveRole);
    confirm->setProperty("variant", "danger");
    QPushButton* keep = box.addButton("Keep Event", QMessageBox::RejectRole);
    box.setDefaultButton(keep);
    box.setEscapeButton(keep);
    box.exec();
    if (box.clickedButton() != confirm) return;

    manager_.deleteEvent(id);
    markDirty();
    refreshAll();
    statusBar()->showMessage(QString("Event %1 deleted").arg(Theme::quoted(name)), 5000);
}

void MainWindow::registerAttendee() {
    AttendeeDialog dialog(this);
    while (dialog.exec() == QDialog::Accepted) {
        try {
            const int id = manager_.registerAttendee(dialog.name().toStdString(), dialog.email().toStdString());
            markDirty();
            refreshAll();
            selectById(attendeesTable_, id);
            statusBar()->showMessage(QString("%1 registered").arg(dialog.name()), 5000);
            return;
        } catch (const std::exception& ex) {
            dialog.showEmailError(friendlyError(ex));
        }
    }
}

void MainWindow::bookSelected() {
    const int eventId = selectedId(eventsTable_);
    const int attendeeId = selectedAttendeeId();
    if (eventId < 0 || attendeeId < 0) return;
    try {
        const Event* e = manager_.findEvent(eventId);
        const QString who = attendeeName(attendeeId);
        const QString what = e ? toQ(e->name()) : QString();
        QString message;
        switch (manager_.book(eventId, attendeeId)) {
            case Event::BookResult::Confirmed: message = QString("%1 booked into %2").arg(who, Theme::quoted(what)); break;
            case Event::BookResult::Waitlisted: message = QString("%1 added to the waitlist for %2").arg(who, Theme::quoted(what)); break;
            case Event::BookResult::Duplicate: message = QString("%1 is already booked or waitlisted for %2").arg(who, Theme::quoted(what)); break;
        }
        markDirty();
        refreshAll();
        statusBar()->showMessage(message, 5000);
    } catch (const std::exception& ex) {
        showError("Couldn't Book Attendee", friendlyError(ex));
    }
}

void MainWindow::cancelSelected() {
    const int eventId = selectedId(eventsTable_);
    const int attendeeId = selectedAttendeeId();
    if (eventId < 0 || attendeeId < 0) return;
    try {
        const QString who = attendeeName(attendeeId);
        const Event::CancelResult result = manager_.cancel(eventId, attendeeId);
        if (!result.cancelled) {
            statusBar()->showMessage(QString("%1 has no booking for this event").arg(who), 5000);
            return;
        }
        QString message = QString("Booking cancelled for %1").arg(who);
        if (result.promotedAttendeeId) message += QString("; %1 moved off the waitlist").arg(attendeeName(*result.promotedAttendeeId));
        markDirty();
        refreshAll();
        statusBar()->showMessage(message, 6000);
    } catch (const std::exception& ex) {
        showError("Couldn't Cancel Booking", friendlyError(ex));
    }
}

void MainWindow::searchEvent() {
    const QString query = eventSearch_->text().trimmed();
    if (query.isEmpty()) return;
    const Event* e = manager_.findEventByName(query.toStdString());
    if (e && selectById(eventsTable_, e->id())) statusBar()->showMessage(QString("Showing %1").arg(Theme::quoted(toQ(e->name()))), 3000);
    else statusBar()->showMessage(QString("No event named %1. Names must match exactly, ignoring case.").arg(Theme::quoted(query)), 5000);
}

void MainWindow::searchAttendee() {
    const QString query = attendeeSearch_->text().trimmed();
    if (query.isEmpty()) return;
    const Attendee* a = manager_.findAttendeeByEmail(query.toStdString());
    if (a && selectById(attendeesTable_, a->id)) statusBar()->showMessage(QString("Showing %1").arg(toQ(a->name)), 3000);
    else statusBar()->showMessage(QString("No attendee with email %1. Emails must match exactly, ignoring case.").arg(Theme::quoted(query)), 5000);
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
        showError("Couldn't Save", QString("%1\n\nCheck that %2 is writable, then save again.").arg(friendlyError(ex), dataPath_));
        return false;
    }
}

void MainWindow::load() {
    if (dirty_) {
        QMessageBox box(QMessageBox::NoIcon, "Discard Changes?", "Reload from file and discard unsaved changes?", QMessageBox::NoButton, this);
        QPushButton* confirm = box.addButton("Discard and Reload", QMessageBox::DestructiveRole);
        confirm->setProperty("variant", "danger");
        QPushButton* keep = box.addButton("Keep Changes", QMessageBox::RejectRole);
        box.setDefaultButton(keep);
        box.setEscapeButton(keep);
        box.exec();
        if (box.clickedButton() != confirm) return;
    }
    try {
        Storage::load(manager_, dataPath_.toStdString());
        dirty_ = false;
        refreshAll();
        updateTitle();
        statusBar()->showMessage("Reloaded from " + dataPath_, 4000);
    } catch (const std::exception& ex) {
        refreshAll();
        showError("Couldn't Reload", QString("%1\n\nCheck that %2 exists and is a valid data file.").arg(friendlyError(ex), dataPath_));
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (!dirty_) {
        event->accept();
        return;
    }
    QMessageBox box(QMessageBox::NoIcon, "Unsaved Changes", "Save changes before closing?", QMessageBox::NoButton, this);
    box.setInformativeText("Your latest events, attendees and bookings haven't been saved.");
    QPushButton* saveButton = box.addButton("Save Changes", QMessageBox::AcceptRole);
    saveButton->setProperty("variant", "primary");
    QPushButton* discard = box.addButton("Discard Changes", QMessageBox::DestructiveRole);
    discard->setProperty("variant", "danger");
    QPushButton* keep = box.addButton("Keep Editing", QMessageBox::RejectRole);
    box.setDefaultButton(saveButton);
    box.setEscapeButton(keep);
    box.exec();

    if (box.clickedButton() == keep || (box.clickedButton() == saveButton && !save())) event->ignore();
    else event->accept();
}

void MainWindow::setDarkMode(bool dark) {
    Theme::apply(*qobject_cast<QApplication*>(QApplication::instance()), dark ? Theme::Mode::Dark : Theme::Mode::Light);
    QSettings().setValue("appearance", dark ? "dark" : "light");
    refreshAll();  // repaints delegate-drawn cells with the new roles
}

void MainWindow::markDirty() {
    dirty_ = true;
    updateTitle();
}

void MainWindow::updateTitle() {
    setWindowTitle(QString("Smart Event Management%1").arg(dirty_ ? " *" : ""));
    if (dirtyChip_) dirtyChip_->setVisible(dirty_);
}

void MainWindow::showError(const QString& title, const QString& message) {
    QMessageBox box(QMessageBox::NoIcon, title, message, QMessageBox::NoButton, this);
    QPushButton* ok = box.addButton("Got It", QMessageBox::AcceptRole);
    box.setDefaultButton(ok);
    box.exec();
}
