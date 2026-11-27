#include "MainWindow.h"
#include "ActionToast.h"
#include "AttendeeDialog.h"
#include "CommandPalette.h"
#include "ConfirmDialog.h"
#include "DateUtil.h"
#include "Delegates.h"
#include "EventDialog.h"
#include "Storage.h"
#include "Theme.h"
#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QCompleter>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>
#include <QProgressBar>
#include <QPushButton>
#include <QResizeEvent>
#include <QTimer>
#include <QScrollArea>
#include <QSettings>
#include <QShowEvent>
#include <QSplitter>
#include <QStackedWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>
#include <exception>

namespace {

constexpr int kPersonAttendeeIdRole = Qt::UserRole + 50;
constexpr int kBookingEventIdRole = Qt::UserRole + 51;
constexpr int kRecordIdRole = Qt::UserRole + 52;

namespace EventCol {
enum { Name = 0, Date = 1, Time = 2, Venue = 3, Booked = 4, Waitlist = 5, Id = 6 };
}  // namespace EventCol

namespace AttendeeCol {
enum { Name = 0, Email = 1, Bookings = 2, Id = 3 };
}  // namespace AttendeeCol

QString toQ(const std::string &s) { return QString::fromStdString(s); }

QString formatDisplayDate(const std::string &isoDate) {
  const QDate d = QDate::fromString(toQ(isoDate), QStringLiteral("yyyy-MM-dd"));
  return d.isValid() ? QLocale().toString(d, QLocale::ShortFormat)
                     : toQ(isoDate);
}

QLabel *makeLabel(const QString &text, const char *role) {
  auto *label = new QLabel(text);
  label->setProperty("role", role);
  return label;
}

QPushButton *makeButton(const QString &text, const char *variant = nullptr) {
  auto *button = new QPushButton(text);
  if (variant)
    button->setProperty("variant", variant);
  button->setFocusPolicy(Qt::TabFocus);
  return button;
}

QFrame *makePanel() {
  auto *panel = new QFrame;
  panel->setObjectName("panel");
  return panel;
}

void setChip(QLabel *chip, const char *kind, const QString &text) {
  chip->setProperty("chip", kind);
  chip->setText(text);
  Theme::repolish(chip);
}

QTableWidget *makeTable(const QStringList &headers) {
  auto *table = new QTableWidget(0, headers.size());
  table->setHorizontalHeaderLabels(headers);
  table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table->setSelectionBehavior(QAbstractItemView::SelectRows);
  table->setSelectionMode(QAbstractItemView::SingleSelection);
  table->setContextMenuPolicy(Qt::CustomContextMenu);
  table->setShowGrid(false);
  table->setFrameShape(QFrame::NoFrame);
  table->setWordWrap(false);
  table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  table->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  table->verticalHeader()->setVisible(false);
  table->verticalHeader()->setDefaultSectionSize(Theme::rowHeight);
  table->horizontalHeader()->setHighlightSections(false);
  table->horizontalHeader()->setSectionsClickable(true);
  table->horizontalHeader()->setSortIndicatorShown(true);
  table->horizontalHeader()->setStretchLastSection(false);
  table->horizontalHeader()->setMinimumSectionSize(56);
  table->setSortingEnabled(true);
  return table;
}

class SortableItem : public QTableWidgetItem {
public:
  using QTableWidgetItem::QTableWidgetItem;
  bool operator<(const QTableWidgetItem &other) const override {
    const QVariant a = data(ItemRole::SortValue);
    const QVariant b = other.data(ItemRole::SortValue);
    if (a.isValid() && b.isValid())
      return a.toLongLong() < b.toLongLong();
    return QTableWidgetItem::operator<(other);
  }
};

void setHeaderAlignment(QTableWidget *table, int column,
                        Qt::Alignment alignment) {
  if (auto *item = table->horizontalHeaderItem(column))
    item->setTextAlignment(alignment | Qt::AlignVCenter);
}

QTableWidgetItem *cell(const QString &text, bool secondary = false,
                       bool right = false, bool mono = false) {
  auto *item = new SortableItem(text);
  item->setData(ItemRole::Tone, secondary ? 1 : 0);
  item->setData(ItemRole::AlignRight, right);
  item->setData(ItemRole::Monospace, mono);
  return item;
}

int selectedId(const QTableWidget *table) {
  int row = table->currentRow();
  if (row < 0 || !table->item(row, 0))
    return -1;
  return table->item(row, 0)->data(kRecordIdRole).toInt();
}

bool selectById(QTableWidget *table, int id) {
  for (int row = 0; row < table->rowCount(); ++row) {
    if (table->item(row, 0) &&
        table->item(row, 0)->data(kRecordIdRole).toInt() == id) {
      table->selectRow(row);
      return true;
    }
  }
  return false;
}

void markSharedPrefixElision(QTableWidget *table, int col) {
  const int rows = table->rowCount();
  for (int i = 0; i < rows; ++i) {
    QTableWidgetItem *a = table->item(i, col);
    if (!a) continue;
    const QString sa = a->text();
    bool shared = sa.size() >= 48;
    if (!shared && sa.size() >= 10) {
      for (int j = 0; j < rows; ++j) {
        if (i == j) continue;
        QTableWidgetItem *b = table->item(j, col);
        if (!b) continue;
        const QString sb = b->text();
        if (sa == sb) continue;
        int common = 0;
        const int lim = qMin(sa.size(), sb.size());
        while (common < lim && sa[common].toLower() == sb[common].toLower())
          ++common;
        if (common >= 10 || (common >= 3 && sa.contains(' ') && sb.contains(' '))) {
          shared = true;
          break;
        }
      }
    }
    a->setData(ItemRole::ElideMode, int(shared ? Qt::ElideMiddle : Qt::ElideRight));
  }
}

QWidget *makeEmptyState(const QString &title, const QString &body,
                        QPushButton *action = nullptr,
                        QLabel **bodyOut = nullptr, bool compact = false) {
  auto *page = new QWidget;
  auto *layout = new QVBoxLayout(page);
  if (compact) {
    layout->setContentsMargins(Theme::space3, Theme::space2, Theme::space3,
                               Theme::space2);
    layout->setSpacing(Theme::space1);
  } else {
    layout->setContentsMargins(Theme::space6, Theme::space6, Theme::space6,
                               Theme::space6);
    layout->setSpacing(Theme::space2);
    layout->addStretch();
  }
  auto *heading = makeLabel(title, "emptyTitle");
  heading->setAlignment(Qt::AlignHCenter);
  auto *text = makeLabel(body, "muted");
  text->setAlignment(Qt::AlignHCenter);
  text->setWordWrap(true);
  if (bodyOut)
    *bodyOut = text;
  layout->addWidget(heading);
  layout->addWidget(text);
  if (action) {
    layout->addSpacing(Theme::space2);
    layout->addWidget(action, 0, Qt::AlignHCenter);
  }
  if (!compact)
    layout->addStretch();
  return page;
}

QComboBox *makeSearchableCombo(const QString &placeholder) {
  auto *combo = new QComboBox;
  combo->setEditable(true);
  combo->setInsertPolicy(QComboBox::NoInsert);
  combo->setMaxVisibleItems(10);
  combo->lineEdit()->setPlaceholderText(placeholder);
  auto *comp = new QCompleter(combo->model(), combo);
  comp->setFilterMode(Qt::MatchContains);
  comp->setCaseSensitivity(Qt::CaseInsensitive);
  comp->setCompletionMode(QCompleter::PopupCompletion);
  combo->setCompleter(comp);
  QObject::connect(
      combo, QOverload<int>::of(&QComboBox::currentIndexChanged), combo,
      [combo](int) {
        QTimer::singleShot(0, combo, [combo] {
          if (combo->lineEdit() && !combo->lineEdit()->hasFocus())
            combo->lineEdit()->setCursorPosition(0);
        });
      });
  return combo;
}

QString friendlyError(const std::exception &ex) {
  const QString raw = QString::fromUtf8(ex.what());
  if (raw == "event name already exists")
    return "An event with that name already exists. Choose a different name.";
  if (raw == "email already registered")
    return "That email is already registered. Use a different email, or search "
           "for the attendee.";
  if (raw.endsWith(" must not be empty")) {
    QString field = raw.left(raw.size() - 18);
    return field.left(1).toUpper() + field.mid(1) +
           " is required. Fill it in and try again.";
  }
  if (raw.endsWith(" contains an invalid character"))
    return "Remove any | characters and line breaks, then try again.";
  if (raw == "event date must be a valid YYYY-MM-DD date")
    return "Enter a valid calendar date in YYYY-MM-DD format.";
  if (raw == "event time must be in hh:mm AM/PM format")
    return "Select a valid hour (1-12), minute (00-59), and AM or PM.";
  if (raw == "capacity cannot be less than confirmed bookings")
    return "Capacity cannot be lower than the number of already confirmed "
           "attendees.";
  if (raw == "attendee email must look like name@example.com")
    return "Enter an email address like name@example.com.";
  return raw;
}

} // namespace

MainWindow::MainWindow(const QString &dataPath, QWidget *parent)
    : QMainWindow(parent), dataPath_(dataPath),
      autoSave_(QSettings().value("autoSave", true).toBool()) {
  buildUi();
  if (QFile::exists(dataPath_)) {
    try {
      Storage::load(manager_, dataPath_.toStdString());
    } catch (const std::exception &ex) {
      enterDegradedMode(friendlyError(ex));
    }
  }
  refreshAll();
  switchView(ViewMode::Events);
  eventsTable_->setFocus();
  updateTitle();
}

int MainWindow::eventRowCount() const { return eventsTable_->rowCount(); }
int MainWindow::eventsPageIndex() const { return eventsStack_->currentIndex(); }
int MainWindow::attendeesPageIndex() const {
  return attendeesStack_->currentIndex();
}

void MainWindow::buildUi() {
  resize(1240, 780);
  setMinimumSize(860, 444);

  registerCommands();
  buildMenus();

  palette_ = new CommandPalette(commands_, this);
  palette_->setJumpProvider([this](const QString &query) {
    QVector<PaletteJumpItem> jumps;
    const QString q = query.trimmed().toLower();
    for (const Event *e : manager_.events()) {
      const QString name = toQ(e->name());
      const QString venue = toQ(e->venue());
      if (name.toLower().contains(q) || venue.toLower().contains(q)) {
        const int eid = e->id();
        jumps.push_back(
            PaletteJumpItem{QStringLiteral("Event"), name,
                            QString("%1 · %2").arg(toQ(e->date()), venue),
                            [this, eid] { jumpToEvent(eid); }});
      }
      if (jumps.size() >= 6)
        break;
    }
    for (const Attendee &a : manager_.attendees()) {
      const QString name = toQ(a.name);
      const QString email = toQ(a.email);
      if (name.toLower().contains(q) || email.toLower().contains(q)) {
        const int aid = a.id;
        jumps.push_back(PaletteJumpItem{QStringLiteral("Attendee"), name, email,
                                        [this, aid] { jumpToAttendee(aid); }});
      }
      if (jumps.size() >= 10)
        break;
    }
    return jumps;
  });
  connect(palette_, &CommandPalette::commandExecuted, this,
          [this](const QString &title, const QString &sc) {
            if (toast_ && !toast_->isVisible())
              toast_->showMessage(title, QString(), {}, sc);
          });

  viewStack_ = new QStackedWidget;
  viewStack_->addWidget(buildEventsView());
  viewStack_->addWidget(buildAttendeesView());

  inspectorOuterStack_ = new QStackedWidget;
  inspectorOuterStack_->addWidget(buildEventInspector());
  inspectorOuterStack_->addWidget(buildAttendeeInspector());

  workspaceSplitter_ = new QSplitter(Qt::Horizontal);
  workspaceSplitter_->setChildrenCollapsible(false);
  workspaceSplitter_->setHandleWidth(Theme::space3);
  workspaceSplitter_->addWidget(viewStack_);
  workspaceSplitter_->addWidget(inspectorOuterStack_);
  workspaceSplitter_->setStretchFactor(0, 3);
  workspaceSplitter_->setStretchFactor(1, 2);
  workspaceSplitter_->setSizes({620, 420});
  connect(workspaceSplitter_, &QSplitter::splitterMoved, this,
          [this](int, int) { updateTableColumns(); });

  auto *rightArea = new QWidget;
  auto *rightLayout = new QVBoxLayout(rightArea);
  rightLayout->setContentsMargins(Theme::space3, Theme::space3, Theme::space3,
                                  Theme::space3);
  rightLayout->setSpacing(Theme::space3);
  rightLayout->addWidget(workspaceSplitter_, 1);

  sidebarRail_ = buildSidebarRail();
  compactBar_ = buildCompactBar();
  compactBar_->hide();
  auto *workspaceRow = new QHBoxLayout;
  workspaceRow->setContentsMargins(0, 0, 0, 0);
  workspaceRow->setSpacing(0);
  workspaceRow->addWidget(sidebarRail_);
  workspaceRow->addWidget(rightArea, 1);

  auto *central = new QWidget;
  central->setObjectName("central");
  auto *rootLayout = new QVBoxLayout(central);
  rootLayout->setContentsMargins(0, 0, 0, 0);
  rootLayout->setSpacing(0);
  rootLayout->addWidget(buildDegradedBanner());
  rootLayout->addWidget(compactBar_);
  rootLayout->addLayout(workspaceRow, 1);
  setCentralWidget(central);

  toast_ = new ActionToast(central);

  connect(eventsTable_, &QTableWidget::itemSelectionChanged, this,
          &MainWindow::refreshDetails);
  connect(attendeesTable_, &QTableWidget::itemSelectionChanged, this,
          &MainWindow::refreshAttendeeDetails);
}

void MainWindow::registerCommands() {
  commands_.registerCommand({"palette.open",
                             "Open Command Palette",
                             {"commands", "search", "find"},
                             "General",
                             QKeySequence(Qt::CTRL | Qt::Key_K),
                             false,
                             nullptr,
                             nullptr,
                             nullptr,
                             [this] { palette_->openPalette(); }});
  commands_.registerCommand({"view.events",
                             "Go to Events",
                             {"events", "list events"},
                             "Navigate",
                             QKeySequence(Qt::CTRL | Qt::Key_1),
                             false,
                             nullptr,
                             nullptr,
                             nullptr,
                             [this] { switchView(ViewMode::Events); }});
  commands_.registerCommand({"view.attendees",
                             "Go to Attendees",
                             {"attendees", "people"},
                             "Navigate",
                             QKeySequence(Qt::CTRL | Qt::Key_2),
                             false,
                             nullptr,
                             nullptr,
                             nullptr,
                             [this] { switchView(ViewMode::Attendees); }});
  commands_.registerCommand({"search.focus",
                             "Focus Filter",
                             {"filter", "search list"},
                             "Navigate",
                             QKeySequence(Qt::Key_Slash),
                             false,
                             nullptr,
                             nullptr,
                             nullptr,
                             [this] { focusActiveSearch(); }});

  commands_.registerCommand(
      {"event.new",
       "New Event",
       {"add event", "create event"},
       "Events",
       QKeySequence::New,
       false,
       [this] { return !degradedMode_; },
       nullptr,
       [this] { return activeView_ == ViewMode::Events ? 25 : 5; },
       [this] { addEvent(); }});
  commands_.registerCommand(
      {"event.edit",
       "Edit Event",
       {"update event", "rename event"},
       "Events",
       QKeySequence(Qt::CTRL | Qt::Key_E),
       false,
       [this] {
         return activeView_ == ViewMode::Events && selectedEventId() >= 0;
       },
       nullptr,
       [this] { return activeView_ == ViewMode::Events ? 30 : 0; },
       [this] { editEvent(); }});
  commands_.registerCommand(
      {"event.delete",
       "Delete Event",
       {"remove event"},
       "Events",
       QKeySequence::Delete,
       false,
       [this] {
         return activeView_ == ViewMode::Events && selectedEventId() >= 0;
       },
       nullptr,
       [this] { return activeView_ == ViewMode::Events ? 15 : 0; },
       [this] { deleteEvent(); }});

  commands_.registerCommand(
      {"attendee.new",
       "Register Attendee",
       {"add attendee", "new person"},
       "Attendees",
       QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N),
       false,
       [this] { return !degradedMode_; },
       nullptr,
       [this] { return activeView_ == ViewMode::Attendees ? 25 : 5; },
       [this] { registerAttendee(); }});
  commands_.registerCommand(
      {"attendee.edit",
       "Edit Attendee",
       {"update attendee", "rename attendee"},
       "Attendees",
       QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E),
       false,
       [this] {
         return activeView_ == ViewMode::Attendees && selectedAttendeeId() >= 0;
       },
       nullptr,
       [this] { return activeView_ == ViewMode::Attendees ? 30 : 0; },
       [this] { editAttendee(); }});
  commands_.registerCommand(
      {"attendee.delete",
       "Delete Attendee",
       {"remove attendee"},
       "Attendees",
       QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Backspace),
       false,
       [this] {
         return activeView_ == ViewMode::Attendees && selectedAttendeeId() >= 0;
       },
       nullptr,
       [this] { return activeView_ == ViewMode::Attendees ? 15 : 0; },
       [this] { deleteAttendee(); }});

  commands_.registerCommand(
      {"booking.book",
       "Book Attendee",
       {"book ticket", "reserve seat"},
       "Booking",
       QKeySequence(Qt::CTRL | Qt::Key_B),
       false,
       [this] {
         if (degradedMode_)
           return false;
         if (activeView_ == ViewMode::Events) {
           const Event *e = manager_.findEvent(selectedEventId());
           const int aid = quickBookAttendeeId();
           return e && aid >= 0 && !e->isConfirmed(aid) && !e->isWaitlisted(aid);
         }
         const Attendee *a = manager_.findAttendee(selectedAttendeeId());
         const Event *e = manager_.findEvent(quickBookEventId());
         return a && e && !e->isConfirmed(a->id) && !e->isWaitlisted(a->id);
       },
       nullptr,
       [] { return 25; },
       [this] {
         if (activeView_ == ViewMode::Events)
           bookFromEventInspector();
         else
           bookFromAttendeeInspector();
       }});
  commands_.registerCommand(
      {"booking.cancel",
       "Cancel Booking",
       {"unbook", "remove booking"},
       "Booking",
       QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_B),
       false,
       [this] {
         if (degradedMode_)
           return false;
         if (activeView_ == ViewMode::Events) {
           const Event *e = manager_.findEvent(selectedEventId());
           const int aid = quickBookAttendeeId();
           return e && aid >= 0 &&
                  (e->isConfirmed(aid) || e->isWaitlisted(aid));
         }
         const Attendee *a = manager_.findAttendee(selectedAttendeeId());
         const Event *e = manager_.findEvent(quickBookEventId());
         if (a && e && (e->isConfirmed(a->id) || e->isWaitlisted(a->id)))
           return true;
         return a && attendeeBookingsList_ && attendeeBookingsList_->currentItem();
       },
       nullptr,
       [] { return 20; },
       [this] {
         if (activeView_ == ViewMode::Events) {
           cancelSelectedBooking();
           return;
         }
         const Event *e = manager_.findEvent(quickBookEventId());
         const Attendee *a = manager_.findAttendee(selectedAttendeeId());
         if (a && e && (e->isConfirmed(a->id) || e->isWaitlisted(a->id))) {
           cancelAttendeeQuickBooking();
         } else if (a && attendeeBookingsList_ &&
                    attendeeBookingsList_->currentItem()) {
           cancelBookingByIds(
               attendeeBookingsList_->currentItem()
                   ->data(kBookingEventIdRole)
                   .toInt(),
               a->id);
         }
       }});

  commands_.registerCommand({"edit.undo",
                             "Undo Last Action",
                             {"revert"},
                             "Edit",
                             QKeySequence::Undo,
                             false,
                             [this] { return undoStack_.canUndo(); },
                             nullptr,
                             [] { return 20; },
                             [this] { undo(); }});
  commands_.registerCommand({"file.save",
                             "Save Data",
                             {"write file"},
                             "File",
                             QKeySequence::Save,
                             false,
                             [this] { return !degradedMode_; },
                             nullptr,
                             nullptr,
                             [this] { save(); }});
  commands_.registerCommand({"file.autosave",
                             "Auto-Save",
                             {"toggle auto save"},
                             "File",
                             QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_S),
                             true,
                             nullptr,
                             [this] { return autoSave_; },
                             nullptr,
                             [this] { setAutoSave(!autoSave_); }});
  commands_.registerCommand({"file.reload",
                             "Reload From File",
                             {"revert file", "load"},
                             "File",
                             QKeySequence(Qt::Key_F5),
                             false,
                             nullptr,
                             nullptr,
                             nullptr,
                             [this] { load(); }});
  commands_.registerCommand({"file.exportCsv",
                             QString::fromUtf8("Export CSV\xE2\x80\xA6"),
                             {"csv", "export"},
                             "File",
                             QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S),
                             false,
                             [this] { return !degradedMode_; },
                             nullptr,
                             nullptr,
                             [this] { exportCsv(); }});
  commands_.registerCommand(
      {"filter.all",
       "Filter: All Events",
       {"show all events", "reset filter"},
       "Events",
       QKeySequence(Qt::ALT | Qt::Key_0),
       false,
       [this] { return !degradedMode_; },
       nullptr,
       [this] { return activeView_ == ViewMode::Events ? 10 : 0; },
       [this] {
         switchView(ViewMode::Events);
         setStatusFilter(StatusFilter::All);
       }});
  commands_.registerCommand(
      {"filter.open",
       "Filter: Open Events",
       {"open events", "available seats"},
       "Events",
       QKeySequence(Qt::ALT | Qt::Key_1),
       false,
       [this] { return !degradedMode_; },
       nullptr,
       [this] { return activeView_ == ViewMode::Events ? 10 : 0; },
       [this] {
         switchView(ViewMode::Events);
         setStatusFilter(StatusFilter::Open);
       }});
  commands_.registerCommand(
      {"filter.almostFull",
       "Filter: Almost Full Events",
       {"almost full", "filling fast"},
       "Events",
       QKeySequence(Qt::ALT | Qt::Key_2),
       false,
       [this] { return !degradedMode_; },
       nullptr,
       [this] { return activeView_ == ViewMode::Events ? 10 : 0; },
       [this] {
         switchView(ViewMode::Events);
         setStatusFilter(StatusFilter::AlmostFull);
       }});
  commands_.registerCommand(
      {"filter.full",
       "Filter: Full Events",
       {"full events", "waitlist events"},
       "Events",
       QKeySequence(Qt::ALT | Qt::Key_3),
       false,
       [this] { return !degradedMode_; },
       nullptr,
       [this] { return activeView_ == ViewMode::Events ? 10 : 0; },
       [this] {
         switchView(ViewMode::Events);
         setStatusFilter(StatusFilter::Full);
       }});
  commands_.registerCommand(
      {"filter.clear",
       "Clear Filter",
       {"reset search", "clear search"},
       "Navigate",
       QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_L),
       false,
       [this] {
         return !degradedMode_ &&
                (statusFilter_ != StatusFilter::All ||
                 (eventSearch_ && !eventSearch_->text().isEmpty()) ||
                 (attendeeSearch_ && !attendeeSearch_->text().isEmpty()));
       },
       nullptr,
       nullptr,
       [this] {
         if (eventSearch_)
           eventSearch_->clear();
         if (attendeeSearch_)
           attendeeSearch_->clear();
         setStatusFilter(StatusFilter::All);
       }});
  commands_.registerCommand(
      {"recovery.restoreBackup",
       "Restore Backup",
       {"recover", "backup", "restore"},
       "Recovery",
       QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R),
       false,
       [this] { return Storage::hasBackup(dataPath_.toStdString()); },
       nullptr,
       [this] { return degradedMode_ ? 30 : 0; },
       [this] { restoreFromBackup(); }});
  commands_.registerCommand(
      {"recovery.chooseFile",
       QString::fromUtf8("Choose Data File\xE2\x80\xA6"),
       {"open file", "switch database", "browse"},
       "Recovery",
       QKeySequence(Qt::CTRL | Qt::Key_O),
       false,
       nullptr,
       nullptr,
       [this] { return degradedMode_ ? 25 : 0; },
       [this] { chooseDataFile(); }});
  commands_.registerCommand(
      {"recovery.startFresh",
       "Start Fresh",
       {"reset data", "empty database", "new file"},
       "Recovery",
       QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Delete),
       false,
       [this] { return degradedMode_; },
       nullptr,
       [this] { return degradedMode_ ? 20 : 0; },
       [this] { startFreshFromCorrupt(); }});
  commands_.registerCommand({"file.quit",
                             "Quit",
                             {"exit", "close"},
                             "File",
                             QKeySequence(Qt::CTRL | Qt::Key_Q),
                             false,
                             nullptr,
                             nullptr,
                             nullptr,
                             [this] { close(); }});

  commands_.registerCommand(
      {"view.darkMode",
       "Dark Mode",
       {"theme", "appearance", "light mode"},
       "View",
       QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D),
       true,
       nullptr,
       [] { return Theme::mode() == Theme::Mode::Dark; },
       nullptr,
       [this] { setDarkMode(Theme::mode() != Theme::Mode::Dark); }});
  commands_.registerCommand(
      {"view.highContrast",
       "High Contrast",
       {"accessibility", "contrast"},
       "View",
       QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_H),
       true,
       nullptr,
       [] { return Theme::highContrast(); },
       nullptr,
       [this] { setHighContrast(!Theme::highContrast()); }});
}

void MainWindow::buildMenus() {
#ifdef Q_OS_MACOS
  auto *fileMenu = menuBar()->addMenu("&File");
  auto *editMenu = menuBar()->addMenu("&Edit");
  auto *viewMenu = menuBar()->addMenu("&View");
#else
  menuBar()->hide();
  appMenu_ = new QMenu(this);
  auto *fileMenu = appMenu_->addMenu("&File");
  auto *editMenu = appMenu_->addMenu("&Edit");
  auto *viewMenu = appMenu_->addMenu("&View");
  connect(appMenu_, &QMenu::aboutToShow, &commands_,
          &CommandRegistry::syncActions);
#endif
  commands_.bindAction("event.new", this, fileMenu);
  commands_.bindAction("attendee.new", this, fileMenu);
  fileMenu->addSeparator();
  commands_.bindAction("file.save", this, fileMenu);
  commands_.bindAction("file.autosave", this, fileMenu);
  commands_.bindAction("file.reload", this, fileMenu);
  commands_.bindAction("recovery.chooseFile", this, fileMenu);
  commands_.bindAction("recovery.restoreBackup", this, fileMenu);
  commands_.bindAction("file.exportCsv", this, fileMenu);
  fileMenu->addSeparator();
  commands_.bindAction("file.quit", this, fileMenu);

  commands_.bindAction("edit.undo", this, editMenu);
  editMenu->addSeparator();
  commands_.bindAction("event.edit", this, editMenu);
  commands_.bindAction("event.delete", this, editMenu);
  commands_.bindAction("attendee.edit", this, editMenu);
  commands_.bindAction("attendee.delete", this, editMenu);
  editMenu->addSeparator();
  commands_.bindAction("booking.book", this, editMenu);
  commands_.bindAction("booking.cancel", this, editMenu);

  commands_.bindAction("palette.open", this, viewMenu);
  viewMenu->addSeparator();
  commands_.bindAction("view.events", this, viewMenu);
  commands_.bindAction("view.attendees", this, viewMenu);
  commands_.bindAction("search.focus", this, viewMenu);
  commands_.bindAction("filter.clear", this, viewMenu);
  viewMenu->addSeparator();
  commands_.bindAction("view.darkMode", this, viewMenu);
  commands_.bindAction("view.highContrast", this, viewMenu);

  connect(fileMenu, &QMenu::aboutToShow, &commands_,
          &CommandRegistry::syncActions);
  connect(editMenu, &QMenu::aboutToShow, &commands_,
          &CommandRegistry::syncActions);
  connect(viewMenu, &QMenu::aboutToShow, &commands_,
          &CommandRegistry::syncActions);
}

QWidget *MainWindow::buildSidebarRail() {
  auto *rail = new QFrame;
  rail->setObjectName("sidebarRail");
  rail->setFixedWidth(220);

  auto *brand = new QLabel("Smart Events");
  brand->setFont(Theme::titleFont(16));
  auto *brandRow = new QHBoxLayout;
  brandRow->setContentsMargins(0, 0, 0, 0);
  brandRow->setSpacing(Theme::space1);
  brandRow->addWidget(brand, 1, Qt::AlignVCenter);
#ifndef Q_OS_MACOS
  appMenuBtn_ = makeButton(QStringLiteral("···"), "icon");
  appMenuBtn_->setFixedSize(32, 32);
  appMenuBtn_->setAccessibleName("Application menu");
  appMenuBtn_->setToolTip("Menu (File, Edit, View)");
  connect(appMenuBtn_, &QPushButton::clicked, this, [this] {
    if (appMenu_)
      appMenu_->popup(
          appMenuBtn_->mapToGlobal(QPoint(0, appMenuBtn_->height() + 4)));
  });
  brandRow->addWidget(appMenuBtn_, 0, Qt::AlignVCenter);
#endif

  railStatusDot_ = new QLabel;
  railStatusDot_->setFixedSize(8, 8);
  railStatusText_ = makeLabel("Saved", "caption");
  auto *statusRow = new QHBoxLayout;
  statusRow->setSpacing(Theme::space2);
  statusRow->addWidget(railStatusDot_, 0, Qt::AlignVCenter);
  statusRow->addWidget(railStatusText_, 1, Qt::AlignVCenter);

  auto makeNavRow = [](const QString &label, QLabel **badgeOut,
                       bool checkable = false) -> QPushButton * {
    auto *btn = new QPushButton;
    btn->setProperty("variant", "nav");
    btn->setMinimumHeight(32);
    btn->setFocusPolicy(Qt::TabFocus);
    btn->setAccessibleName(label);
    if (checkable)
      btn->setCheckable(true);
    auto *row = new QHBoxLayout(btn);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(Theme::space2);
    auto *textLabel = makeLabel(label, "navText");
    row->addWidget(textLabel, 1, Qt::AlignVCenter);
    auto *badge = new QLabel("0");
    badge->setProperty("chip", "neutral");
    badge->setFont(Theme::monoFont(11));
    badge->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    row->addWidget(badge, 0, Qt::AlignVCenter);
    if (badgeOut)
      *badgeOut = badge;
    return btn;
  };

  navEventsBtn_ = makeNavRow("Events", &navEventsBadge_);
  navAttendeesBtn_ = makeNavRow("Attendees", &navAttendeesBadge_);
  connect(navEventsBtn_, &QPushButton::clicked, this,
          [this] { switchView(ViewMode::Events); });
  connect(navAttendeesBtn_, &QPushButton::clicked, this,
          [this] { switchView(ViewMode::Attendees); });

  auto *paletteBtn = new QPushButton;
  paletteBtn->setProperty("variant", "nav");
  paletteBtn->setMinimumHeight(32);
  paletteBtn->setFocusPolicy(Qt::TabFocus);
  paletteBtn->setAccessibleName("Open Command Palette");
  auto *palLayout = new QHBoxLayout(paletteBtn);
  palLayout->setContentsMargins(0, 0, 0, 0);
  palLayout->setSpacing(Theme::space2);
  palLayout->addWidget(makeLabel("Commands", "navText"), 1, Qt::AlignVCenter);
  auto *kbd = makeLabel("Ctrl+K", "kbd");
  kbd->setFont(Theme::monoFont(11));
  kbd->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  palLayout->addWidget(kbd, 0, Qt::AlignVCenter);
  connect(paletteBtn, &QPushButton::clicked, this,
          [this] { palette_->openPalette(); });

  railAutoSaveBtn_ = makeNavRow("Auto-Save", &railAutoSaveBadge_, true);
  connect(railAutoSaveBtn_, &QPushButton::toggled, this,
          [this](bool on) { setAutoSave(on); });

  railDarkBtn_ = makeNavRow("Dark Mode", &railDarkBadge_, true);
  connect(railDarkBtn_, &QPushButton::toggled, this,
          [this](bool on) { setDarkMode(on); });

  railContrastBtn_ = makeNavRow("High Contrast", &railContrastBadge_, true);
  connect(railContrastBtn_, &QPushButton::toggled, this,
          [this](bool on) { setHighContrast(on); });

  auto *layout = new QVBoxLayout(rail);
  layout->setContentsMargins(Theme::space3, Theme::space4, Theme::space3,
                             Theme::space4);
  layout->setSpacing(Theme::space1);
  layout->addLayout(brandRow);
  layout->addLayout(statusRow);
  layout->addSpacing(Theme::space4);
  layout->addWidget(navEventsBtn_);
  layout->addWidget(navAttendeesBtn_);
  layout->addStretch();
  layout->addWidget(paletteBtn);
  layout->addWidget(railAutoSaveBtn_);
  layout->addWidget(railDarkBtn_);
  layout->addWidget(railContrastBtn_);
  return rail;
}

QWidget *MainWindow::buildCompactBar() {
  auto *bar = new QFrame;
  bar->setObjectName("compactBar");
  bar->setFixedHeight(42);

  auto *brand = new QLabel("Smart Events");
  brand->setFont(Theme::titleFont(14));

  compactStatusDot_ = new QLabel;
  compactStatusDot_->setFixedSize(8, 8);

  compactEventsBtn_ = makeButton("Events", "nav");
  compactEventsBtn_->setAccessibleName("Events");
  compactEventsBtn_->setFocusPolicy(Qt::TabFocus);
  connect(compactEventsBtn_, &QPushButton::clicked, this,
          [this] { switchView(ViewMode::Events); });

  compactAttendeesBtn_ = makeButton("Attendees", "nav");
  compactAttendeesBtn_->setAccessibleName("Attendees");
  compactAttendeesBtn_->setFocusPolicy(Qt::TabFocus);
  connect(compactAttendeesBtn_, &QPushButton::clicked, this,
          [this] { switchView(ViewMode::Attendees); });

  auto *cmdBtn = makeButton("Commands", "ghost");
  cmdBtn->setAccessibleName("Open Command Palette");
  cmdBtn->setToolTip("Open Command Palette (Ctrl+K)");
  connect(cmdBtn, &QPushButton::clicked, this,
          [this] { palette_->openPalette(); });

  compactMenuBtn_ = makeButton(QStringLiteral("···"), "icon");
  compactMenuBtn_->setFixedSize(32, 32);
  compactMenuBtn_->setAccessibleName("Application menu");
  compactMenuBtn_->setToolTip("Menu (File, Edit, View)");
  connect(compactMenuBtn_, &QPushButton::clicked, this, [this] {
    if (appMenu_)
      appMenu_->popup(
          compactMenuBtn_->mapToGlobal(QPoint(0, compactMenuBtn_->height() + 4)));
  });

  auto *layout = new QHBoxLayout(bar);
  layout->setContentsMargins(Theme::space3, Theme::space1, Theme::space3,
                             Theme::space1);
  layout->setSpacing(Theme::space2);
  layout->addWidget(brand);
  layout->addWidget(compactStatusDot_);
  layout->addSpacing(Theme::space1);
  layout->addWidget(compactEventsBtn_);
  layout->addWidget(compactAttendeesBtn_);
  layout->addStretch();
  layout->addWidget(cmdBtn);
  layout->addWidget(compactMenuBtn_);
  return bar;
}

QWidget *MainWindow::buildDegradedBanner() {
  degradedBanner_ = new QFrame;
  degradedBanner_->setObjectName("degradedBanner");
  degradedBanner_->setProperty("severity", "warning");
  degradedBanner_->hide();

  degradedLabel_ = new QLabel;
  degradedLabel_->setWordWrap(true);

  auto *reloadBtn = makeButton("Reload File");
  restoreBackupBtn_ = makeButton("Restore Backup");
  restoreBackupBtn_->installEventFilter(this);
  auto *chooseBtn = makeButton(QString::fromUtf8("Choose File\xE2\x80\xA6"), "ghost");
  auto *freshBtn = makeButton("Start Fresh", "danger");
  connect(reloadBtn, &QPushButton::clicked, this, &MainWindow::load);
  connect(restoreBackupBtn_, &QPushButton::clicked, this,
          &MainWindow::restoreFromBackup);
  connect(chooseBtn, &QPushButton::clicked, this, &MainWindow::chooseDataFile);
  connect(freshBtn, &QPushButton::clicked, this,
          &MainWindow::startFreshFromCorrupt);

  auto *layout = new QHBoxLayout(degradedBanner_);
  layout->setContentsMargins(Theme::space3, Theme::space2, Theme::space3,
                             Theme::space2);
  layout->setSpacing(Theme::space2);
  layout->addWidget(degradedLabel_, 1);
  layout->addWidget(reloadBtn);
  layout->addWidget(restoreBackupBtn_);
  layout->addWidget(chooseBtn);
  layout->addWidget(freshBtn);
  return degradedBanner_;
}

QWidget *MainWindow::buildEventsView() {
  auto *title = new QLabel("Events");
  title->setFont(Theme::titleFont(20));
  eventsCount_ = makeLabel("0", "caption");

  eventsSegFrame_ = new QFrame;
  eventsSegFrame_->setObjectName("segmented");
  eventsSegFrame_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  auto *segLayout = new QHBoxLayout(eventsSegFrame_);
  segLayout->setContentsMargins(0, 0, 0, 0);
  segLayout->setSpacing(2);
  const char *segLabels[4] = {"All", "Open", "Almost Full", "Full"};
  for (int i = 0; i < 4; ++i) {
    segFilterButtons_[i] = makeButton(segLabels[i], "segment");
    segFilterButtons_[i]->setCheckable(true);
    segFilterButtons_[i]->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(segFilterButtons_[i], &QPushButton::clicked, this,
            [this, i] { setStatusFilter(static_cast<StatusFilter>(i)); });
    segLayout->addWidget(segFilterButtons_[i]);
  }

  eventsNewBtn_ = makeButton("New Event", "primary");
  connect(eventsNewBtn_, &QPushButton::clicked, this, &MainWindow::addEvent);

  auto *headerRow = new QHBoxLayout;
  headerRow->setSpacing(Theme::space2);
  headerRow->addWidget(title);
  headerRow->addWidget(eventsCount_, 0, Qt::AlignBaseline);
  headerRow->addStretch();
  headerRow->addWidget(eventsSegFrame_);
  headerRow->addSpacing(Theme::space1);
  headerRow->addWidget(eventsNewBtn_);
  eventsNewBtn_->setToolTip("New Event (Ctrl+N)");

  eventSearch_ = new QLineEdit;
  eventSearch_->setPlaceholderText(
      "Filter events by name, date, time, or venue  (/)");
  eventSearch_->setClearButtonEnabled(true);
  connect(eventSearch_, &QLineEdit::textChanged, this,
          &MainWindow::filterEvents);
  eventSearch_->setAccessibleName("Filter events");
  connect(eventSearch_, &QLineEdit::returnPressed, this,
          &MainWindow::searchEvent);

  eventsTable_ =
      makeTable({"Name", "Date", "Time", "Venue", "Booked", "Waitlist", "ID"});
  eventsTable_->setAccessibleName("Events table");
  eventsTable_->setMinimumHeight(200);
  auto *header = eventsTable_->horizontalHeader();
  header->setSectionResizeMode(EventCol::Name, QHeaderView::Fixed);
  header->setSectionResizeMode(EventCol::Date, QHeaderView::Fixed);
  header->setSectionResizeMode(EventCol::Time, QHeaderView::Fixed);
  header->setSectionResizeMode(EventCol::Venue, QHeaderView::Stretch);
  header->setSectionResizeMode(EventCol::Booked, QHeaderView::Fixed);
  header->setSectionResizeMode(EventCol::Waitlist, QHeaderView::Fixed);
  header->setSectionResizeMode(EventCol::Id, QHeaderView::Fixed);
  eventsTable_->setColumnWidth(EventCol::Name, 180);
  eventsTable_->setColumnWidth(EventCol::Date, 98);
  eventsTable_->setColumnWidth(EventCol::Time, 80);
  eventsTable_->setColumnWidth(EventCol::Booked, 144);
  eventsTable_->setColumnWidth(EventCol::Waitlist, 66);
  eventsTable_->setColumnWidth(EventCol::Id, 56);
  setHeaderAlignment(eventsTable_, EventCol::Name, Qt::AlignLeft);
  setHeaderAlignment(eventsTable_, EventCol::Waitlist, Qt::AlignRight);
  setHeaderAlignment(eventsTable_, EventCol::Id, Qt::AlignRight);
  trackHover(eventsTable_, new TableDelegate(eventsTable_));
  connect(eventsTable_, &QTableWidget::itemDoubleClicked, this,
          [this] { editEvent(); });
  connect(eventsTable_, &QWidget::customContextMenuRequested, this,
          [this](const QPoint &pos) {
            if (eventsTable_->indexAt(pos).isValid())
              eventsTable_->selectRow(eventsTable_->indexAt(pos).row());
            if (selectedEventId() < 0)
              return;
            QMenu menu(this);
            commands_.syncActions();
            commands_.bindAction("event.edit", this, &menu);
            commands_.bindAction("event.delete", this, &menu);
            menu.exec(eventsTable_->viewport()->mapToGlobal(pos));
          });

  auto *emptyAdd = makeButton("New Event");
  connect(emptyAdd, &QPushButton::clicked, this, &MainWindow::addEvent);
  auto *clearFilterBtn = makeButton("Clear Filter");
  connect(clearFilterBtn, &QPushButton::clicked, this, [this] {
    eventSearch_->clear();
    setStatusFilter(StatusFilter::All);
  });

  eventsStack_ = new QStackedWidget;
  eventsStack_->addWidget(makeEmptyState(
      "No events yet", "Create your first event, then book attendees into it.",
      emptyAdd));
  eventsStack_->addWidget(makeEmptyState(
      "No matching events", "Adjust your search or status filter.",
      clearFilterBtn, &eventsFilterEmptyLabel_));
  eventsStack_->addWidget(makeEmptyState(
      "Events unavailable",
      "Your data file could not be loaded. Use the recovery actions above."));
  eventsStack_->addWidget(eventsTable_);

  auto *panel = makePanel();
  auto *layout = new QVBoxLayout(panel);
  layout->setContentsMargins(Theme::space4, Theme::space4, Theme::space4,
                             Theme::space3);
  layout->setSpacing(Theme::space3);
  layout->addLayout(headerRow);
  layout->addWidget(eventSearch_);
  layout->addWidget(eventsStack_, 1);
  return panel;
}

QWidget *MainWindow::buildAttendeesView() {
  auto *title = new QLabel("Attendees");
  title->setFont(Theme::titleFont(20));
  attendeesCount_ = makeLabel("0", "caption");

  attendeesRegisterBtn_ = makeButton("Register Attendee", "primary");
  connect(attendeesRegisterBtn_, &QPushButton::clicked, this,
          &MainWindow::registerAttendee);

  auto *headerRow = new QHBoxLayout;
  headerRow->setSpacing(Theme::space2);
  headerRow->addWidget(title);
  headerRow->addWidget(attendeesCount_, 0, Qt::AlignBaseline);
  headerRow->addStretch();
  headerRow->addWidget(attendeesRegisterBtn_);
  attendeesRegisterBtn_->setToolTip("Register Attendee (Ctrl+Shift+N)");

  attendeeSearch_ = new QLineEdit;
  attendeeSearch_->setAccessibleName("Filter attendees");
  attendeeSearch_->setPlaceholderText("Filter attendees by name or email  (/)");
  attendeeSearch_->setClearButtonEnabled(true);
  connect(attendeeSearch_, &QLineEdit::textChanged, this,
          &MainWindow::filterAttendees);
  connect(attendeeSearch_, &QLineEdit::returnPressed, this,
          &MainWindow::searchAttendee);

  attendeesTable_ = makeTable({"Name", "Email", "Bookings", "ID"});
  attendeesTable_->setAccessibleName("Attendees table");
  attendeesTable_->setMinimumHeight(200);
  auto *header = attendeesTable_->horizontalHeader();
  header->setSectionResizeMode(AttendeeCol::Name, QHeaderView::Fixed);
  header->setSectionResizeMode(AttendeeCol::Email, QHeaderView::Stretch);
  header->setSectionResizeMode(AttendeeCol::Bookings, QHeaderView::Fixed);
  header->setSectionResizeMode(AttendeeCol::Id, QHeaderView::Fixed);
  attendeesTable_->setColumnWidth(AttendeeCol::Name, 180);
  attendeesTable_->setColumnWidth(AttendeeCol::Bookings, 118);
  attendeesTable_->setColumnWidth(AttendeeCol::Id, 56);
  setHeaderAlignment(attendeesTable_, AttendeeCol::Name, Qt::AlignLeft);
  setHeaderAlignment(attendeesTable_, AttendeeCol::Bookings, Qt::AlignRight);
  setHeaderAlignment(attendeesTable_, AttendeeCol::Id, Qt::AlignRight);
  trackHover(attendeesTable_, new TableDelegate(attendeesTable_));
  connect(attendeesTable_, &QTableWidget::itemDoubleClicked, this,
          [this] { editAttendee(); });
  connect(attendeesTable_, &QWidget::customContextMenuRequested, this,
          [this](const QPoint &pos) {
            if (attendeesTable_->indexAt(pos).isValid())
              attendeesTable_->selectRow(attendeesTable_->indexAt(pos).row());
            if (selectedAttendeeId() < 0)
              return;
            QMenu menu(this);
            commands_.syncActions();
            commands_.bindAction("attendee.edit", this, &menu);
            commands_.bindAction("attendee.delete", this, &menu);
            menu.exec(attendeesTable_->viewport()->mapToGlobal(pos));
          });

  auto *emptyAdd = makeButton("Register Attendee");
  connect(emptyAdd, &QPushButton::clicked, this, &MainWindow::registerAttendee);
  auto *clearFilterBtn = makeButton("Clear Filter");
  connect(clearFilterBtn, &QPushButton::clicked, this,
          [this] { attendeeSearch_->clear(); });

  attendeesStack_ = new QStackedWidget;
  attendeesStack_->addWidget(
      makeEmptyState("No attendees yet",
                     "Register someone to start taking bookings.", emptyAdd));
  attendeesStack_->addWidget(
      makeEmptyState("No matching attendees", "Try a different name or email.",
                     clearFilterBtn, &attendeesFilterEmptyLabel_));
  attendeesStack_->addWidget(makeEmptyState(
      "Attendees unavailable",
      "Your data file could not be loaded. Use the recovery actions above."));
  attendeesStack_->addWidget(attendeesTable_);

  auto *panel = makePanel();
  auto *layout = new QVBoxLayout(panel);
  layout->setContentsMargins(Theme::space4, Theme::space4, Theme::space4,
                             Theme::space3);
  layout->setSpacing(Theme::space3);
  layout->addLayout(headerRow);
  layout->addWidget(attendeeSearch_);
  layout->addWidget(attendeesStack_, 1);
  return panel;
}

QWidget *MainWindow::buildEventInspector() {
  auto *none = makeEmptyState("No event selected",
                              "Select an event to inspect capacity, book "
                              "attendees, or manage the waitlist.");

  eventTitle_ = new QLabel;
  eventTitle_->setFont(Theme::titleFont(20));
  eventTitle_->setWordWrap(true);
  editEventBtn_ = makeButton("Edit", "ghost");
  editEventBtn_->setAccessibleName("Edit Event");
  editEventBtn_->setToolTip("Edit Event (Ctrl+E)");
  deleteEventBtn_ = makeButton("Delete Event", "danger");
  deleteEventBtn_->setAccessibleName("Delete Event");
  deleteEventBtn_->setToolTip("Delete Event (Del)");
  connect(editEventBtn_, &QPushButton::clicked, this, &MainWindow::editEvent);
  connect(deleteEventBtn_, &QPushButton::clicked, this,
          &MainWindow::deleteEvent);

  auto *titleRow = new QHBoxLayout;
  titleRow->setSpacing(Theme::space1);
  titleRow->addWidget(eventTitle_, 1);
  titleRow->addWidget(editEventBtn_);

  eventMeta_ = makeLabel("", "muted");
  eventMeta_->setWordWrap(true);
  eventDescription_ = makeLabel("", "caption");
  eventDescription_->setWordWrap(true);
  eventDescription_->hide();

  auto *headerGroup = new QVBoxLayout;
  headerGroup->setSpacing(Theme::space1);
  headerGroup->addLayout(titleRow);
  headerGroup->addWidget(eventMeta_);
  headerGroup->addWidget(eventDescription_);

  seatsLabel_ = new QLabel;
  statusChip_ = new QLabel;
  setChip(statusChip_, "success", "Open");
  seatMeter_ = new QProgressBar;
  seatMeter_->setTextVisible(false);
  auto *seatRow = new QHBoxLayout;
  seatRow->addWidget(seatsLabel_);
  seatRow->addStretch();
  seatRow->addWidget(statusChip_);

  quickBookCombo_ = makeSearchableCombo(
      QString::fromUtf8("Search attendee by name or email\xE2\x80\xA6"));
  quickBookCombo_->setAccessibleName("Select attendee to book");
  quickBookCombo_->lineEdit()->setAccessibleName("Search attendee to book");
  connect(quickBookCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &MainWindow::updateActions);
  connect(quickBookCombo_->lineEdit(), &QLineEdit::textChanged, this,
          &MainWindow::updateActions);

  bookButton_ = makeButton("Book Attendee", "primary");
  bookButton_->setToolTip("Book Attendee (Ctrl+B)");
  cancelButton_ = makeButton("Cancel Booking");
  cancelButton_->setToolTip("Cancel Booking (Ctrl+Shift+B)");
  connect(bookButton_, &QPushButton::clicked, this,
          &MainWindow::bookFromEventInspector);
  connect(cancelButton_, &QPushButton::clicked, this,
          &MainWindow::cancelSelectedBooking);

  actionHint_ = makeLabel("", "caption");
  actionHint_->setWordWrap(true);

  quickBookStateChip_ = new QLabel;
  setChip(quickBookStateChip_, "success", "Booked");
  quickBookStateChip_->hide();

  auto *wellButtons = new QHBoxLayout;
  wellButtons->setSpacing(Theme::space2);
  wellButtons->addWidget(quickBookStateChip_, 0, Qt::AlignVCenter);
  wellButtons->addWidget(bookButton_);
  wellButtons->addWidget(cancelButton_);
  wellButtons->addStretch();

  auto *well = new QFrame;
  well->setObjectName("well");
  auto *wellLayout = new QVBoxLayout(well);
  wellLayout->setContentsMargins(Theme::space3, Theme::space3, Theme::space3,
                                 Theme::space3);
  wellLayout->setSpacing(Theme::space2);
  wellLayout->addWidget(makeLabel("Quick Book", "fieldLabel"));
  wellLayout->addWidget(quickBookCombo_);
  wellLayout->addWidget(actionHint_);
  wellLayout->addLayout(wellButtons);

  confirmedCount_ = makeLabel("0", "caption");
  confirmedList_ = new QListWidget;
  confirmedList_->setAccessibleName("Confirmed attendees");
  confirmedList_->setSelectionMode(QAbstractItemView::SingleSelection);
  confirmedList_->setFrameShape(QFrame::NoFrame);
  confirmedList_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  trackHover(confirmedList_, new PersonDelegate(confirmedList_));
  confirmedStack_ = new QStackedWidget;
  confirmedStack_->addWidget(
      makeEmptyState("No confirmed attendees",
                     "Pick an attendee above and click Book Attendee.", nullptr,
                     nullptr, true));
  confirmedStack_->addWidget(confirmedList_);

  waitlistCount_ = makeLabel("0", "caption");
  waitlistList_ = new QListWidget;
  waitlistList_->setAccessibleName("Waitlist attendees");
  waitlistList_->setSelectionMode(QAbstractItemView::SingleSelection);
  waitlistList_->setFrameShape(QFrame::NoFrame);
  waitlistList_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  trackHover(waitlistList_, new PersonDelegate(waitlistList_));
  waitlistStack_ = new QStackedWidget;
  waitlistStack_->addWidget(makeEmptyState(
      "Waitlist is empty",
      "Attendees who book when the event is full wait here in order.", nullptr,
      nullptr, true));
  waitlistStack_->addWidget(waitlistList_);

  connect(confirmedList_, &QListWidget::itemDoubleClicked, this,
          [this](QListWidgetItem *item) {
            jumpToAttendee(item->data(kPersonAttendeeIdRole).toInt());
          });
  connect(waitlistList_, &QListWidget::itemDoubleClicked, this,
          [this](QListWidgetItem *item) {
            jumpToAttendee(item->data(kPersonAttendeeIdRole).toInt());
          });

  auto *confirmedHeader = new QHBoxLayout;
  confirmedHeader->setSpacing(Theme::space2);
  confirmedHeader->addWidget(makeLabel("Confirmed", "panelTitle"));
  confirmedHeader->addWidget(confirmedCount_, 0, Qt::AlignBaseline);
  confirmedHeader->addStretch();

  auto *waitlistHeader = new QHBoxLayout;
  waitlistHeader->setSpacing(Theme::space2);
  waitlistHeader->addWidget(makeLabel("Waitlist", "panelTitle"));
  waitlistHeader->addWidget(waitlistCount_, 0, Qt::AlignBaseline);
  waitlistHeader->addStretch();

  auto *detail = new QWidget;
  eventDetailLayout_ = new QVBoxLayout(detail);
  eventDetailLayout_->setContentsMargins(0, 0, 0, 0);
  eventDetailLayout_->setSpacing(Theme::space3);
  eventDetailLayout_->addLayout(headerGroup);
  eventDetailLayout_->addLayout(seatRow);
  eventDetailLayout_->addWidget(seatMeter_);
  eventDetailLayout_->addWidget(well);
  eventDetailLayout_->addLayout(confirmedHeader);
  eventDetailLayout_->addWidget(confirmedStack_, 1);
  eventDetailLayout_->addLayout(waitlistHeader);
  eventDetailLayout_->addWidget(waitlistStack_, 0);
  detailTailSpacer_ = new QWidget;
  detailTailSpacer_->setSizePolicy(QSizePolicy::Preferred,
                                   QSizePolicy::Expanding);
  eventDetailLayout_->addWidget(detailTailSpacer_, 1);
  auto *eventDangerRow = new QHBoxLayout;
  eventDangerRow->setContentsMargins(0, Theme::space2, 0, 0);
  eventDangerRow->addWidget(deleteEventBtn_);
  eventDangerRow->addStretch();
  eventDetailLayout_->addLayout(eventDangerRow);

  auto *detailScroll = new QScrollArea;
  detailScroll->setFocusPolicy(Qt::NoFocus);
  detailScroll->viewport()->setFocusPolicy(Qt::NoFocus);
  detailScroll->setWidgetResizable(true);
  detailScroll->setFrameShape(QFrame::NoFrame);
  detailScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  detailScroll->setWidget(detail);

  eventInspectorStack_ = new QStackedWidget;
  eventInspectorStack_->addWidget(none);
  eventInspectorStack_->addWidget(detailScroll);

  eventInspectorPanel_ = makePanel();
  auto *outer = new QVBoxLayout(eventInspectorPanel_);
  outer->setContentsMargins(Theme::space4, Theme::space4, Theme::space4,
                            Theme::space4);
  outer->addWidget(eventInspectorStack_);
  eventInspectorPanel_->setMinimumWidth(340);
  eventInspectorPanel_->setMaximumWidth(480);
  return eventInspectorPanel_;
}

QWidget *MainWindow::buildAttendeeInspector() {
  auto *none = makeEmptyState(
      "No attendee selected",
      "Select an attendee to view or manage their event bookings.");

  attendeeTitle_ = new QLabel;
  attendeeTitle_->setFont(Theme::titleFont(20));
  attendeeTitle_->setWordWrap(true);
  editAttendeeBtn_ = makeButton("Edit", "ghost");
  editAttendeeBtn_->setAccessibleName("Edit Attendee");
  editAttendeeBtn_->setToolTip("Edit Attendee (Ctrl+Shift+E)");
  deleteAttendeeBtn_ = makeButton("Delete Attendee", "danger");
  deleteAttendeeBtn_->setAccessibleName("Delete Attendee");
  deleteAttendeeBtn_->setToolTip("Delete Attendee (Ctrl+Shift+Backspace)");
  connect(editAttendeeBtn_, &QPushButton::clicked, this,
          &MainWindow::editAttendee);
  connect(deleteAttendeeBtn_, &QPushButton::clicked, this,
          &MainWindow::deleteAttendee);

  auto *titleRow = new QHBoxLayout;
  titleRow->setSpacing(Theme::space1);
  titleRow->addWidget(attendeeTitle_, 1);
  titleRow->addWidget(editAttendeeBtn_);

  attendeeEmail_ = makeLabel("", "muted");
  attendeeSummary_ = makeLabel("", "caption");

  auto *headerGroup = new QVBoxLayout;
  headerGroup->setSpacing(Theme::space1);
  headerGroup->addLayout(titleRow);
  headerGroup->addWidget(attendeeEmail_);
  headerGroup->addWidget(attendeeSummary_);

  quickEventCombo_ = makeSearchableCombo(
      QString::fromUtf8("Search event by name or date\xE2\x80\xA6"));
  quickEventCombo_->setAccessibleName("Select event to book");
  quickEventCombo_->lineEdit()->setAccessibleName("Search event to book");
  connect(quickEventCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &MainWindow::updateActions);
  connect(quickEventCombo_->lineEdit(), &QLineEdit::textChanged, this,
          &MainWindow::updateActions);

  attendeeActionHint_ = makeLabel("", "caption");
  attendeeActionHint_->setWordWrap(true);

  attendeeBookStateChip_ = new QLabel;
  setChip(attendeeBookStateChip_, "success", "Booked");
  attendeeBookStateChip_->hide();

  bookEventBtn_ = makeButton("Book Event", "primary");
  bookEventBtn_->setToolTip("Book Event (Ctrl+B)");
  connect(bookEventBtn_, &QPushButton::clicked, this,
          &MainWindow::bookFromAttendeeInspector);

  cancelEventBookingBtn_ = makeButton("Cancel Booking");
  cancelEventBookingBtn_->setToolTip("Cancel Booking (Ctrl+Shift+B)");
  connect(cancelEventBookingBtn_, &QPushButton::clicked, this,
          &MainWindow::cancelAttendeeQuickBooking);

  auto *wellButtons = new QHBoxLayout;
  wellButtons->setSpacing(Theme::space2);
  wellButtons->addWidget(attendeeBookStateChip_, 0, Qt::AlignVCenter);
  wellButtons->addWidget(bookEventBtn_);
  wellButtons->addWidget(cancelEventBookingBtn_);
  wellButtons->addStretch();

  auto *well = new QFrame;
  well->setObjectName("well");
  auto *wellLayout = new QVBoxLayout(well);
  wellLayout->setContentsMargins(Theme::space3, Theme::space3, Theme::space3,
                                 Theme::space3);
  wellLayout->setSpacing(Theme::space2);
  wellLayout->addWidget(makeLabel("Book into Event", "fieldLabel"));
  wellLayout->addWidget(quickEventCombo_);
  wellLayout->addWidget(attendeeActionHint_);
  wellLayout->addLayout(wellButtons);

  attendeeBookingsCount_ = makeLabel("0", "caption");
  attendeeBookingsList_ = new QListWidget;
  attendeeBookingsList_->setAccessibleName("Attendee bookings");
  attendeeBookingsList_->setSelectionMode(QAbstractItemView::SingleSelection);
  attendeeBookingsList_->setFrameShape(QFrame::NoFrame);
  attendeeBookingsList_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  trackHover(attendeeBookingsList_, new PersonDelegate(attendeeBookingsList_));
  attendeeBookingsStack_ = new QStackedWidget;
  attendeeBookingsStack_->addWidget(makeEmptyState(
      "No bookings yet", "Choose an event above to book this attendee.",
      nullptr, nullptr, true));
  attendeeBookingsStack_->addWidget(attendeeBookingsList_);

  connect(attendeeBookingsList_, &QListWidget::itemDoubleClicked, this,
          [this](QListWidgetItem *item) {
            jumpToEvent(item->data(kBookingEventIdRole).toInt());
          });

  auto *bookingsHeader = new QHBoxLayout;
  bookingsHeader->setSpacing(Theme::space2);
  bookingsHeader->addWidget(makeLabel("Bookings", "panelTitle"));
  bookingsHeader->addWidget(attendeeBookingsCount_, 0, Qt::AlignBaseline);
  bookingsHeader->addStretch();

  auto *detail = new QWidget;
  attendeeDetailLayout_ = new QVBoxLayout(detail);
  attendeeDetailLayout_->setContentsMargins(0, 0, 0, 0);
  attendeeDetailLayout_->setSpacing(Theme::space3);
  attendeeDetailLayout_->addLayout(headerGroup);
  attendeeDetailLayout_->addWidget(well);
  attendeeDetailLayout_->addLayout(bookingsHeader);
  attendeeDetailLayout_->addWidget(attendeeBookingsStack_, 0);
  attendeeTailSpacer_ = new QWidget;
  attendeeTailSpacer_->setSizePolicy(QSizePolicy::Preferred,
                                     QSizePolicy::Expanding);
  attendeeDetailLayout_->addWidget(attendeeTailSpacer_, 1);
  auto *attendeeDangerRow = new QHBoxLayout;
  attendeeDangerRow->setContentsMargins(0, Theme::space2, 0, 0);
  attendeeDangerRow->addWidget(deleteAttendeeBtn_);
  attendeeDangerRow->addStretch();
  attendeeDetailLayout_->addLayout(attendeeDangerRow);

  auto *detailScroll = new QScrollArea;
  detailScroll->setFocusPolicy(Qt::NoFocus);
  detailScroll->viewport()->setFocusPolicy(Qt::NoFocus);
  detailScroll->setWidgetResizable(true);
  detailScroll->setFrameShape(QFrame::NoFrame);
  detailScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  detailScroll->setWidget(detail);

  attendeeInspectorStack_ = new QStackedWidget;
  attendeeInspectorStack_->addWidget(none);
  attendeeInspectorStack_->addWidget(detailScroll);

  attendeeInspectorPanel_ = makePanel();
  auto *outer = new QVBoxLayout(attendeeInspectorPanel_);
  outer->setContentsMargins(Theme::space4, Theme::space4, Theme::space4,
                            Theme::space4);
  outer->addWidget(attendeeInspectorStack_);
  attendeeInspectorPanel_->setMinimumWidth(340);
  attendeeInspectorPanel_->setMaximumWidth(480);
  return attendeeInspectorPanel_;
}

void MainWindow::trackHover(QAbstractItemView *view, HoverDelegate *delegate) {
  view->setItemDelegate(delegate);
  view->setMouseTracking(true);
  view->viewport()->installEventFilter(this);
  hoverViewports_.insert(view->viewport(), delegate);
  connect(view, &QAbstractItemView::entered, this,
          [view, delegate](const QModelIndex &index) {
            delegate->hoverRow = index.row();
            view->viewport()->update();
          });
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
  if (watched == restoreBackupBtn_ && event->type() == QEvent::FocusIn &&
      restoreBackupBtn_->property("inactive").toBool() && toast_) {
    toast_->showMessage(QString("No backup file (%1.bak) exists to restore")
                            .arg(QFileInfo(dataPath_).fileName()));
  }
  if (event->type() == QEvent::Leave) {
    auto it = hoverViewports_.constFind(watched);
    if (it != hoverViewports_.constEnd()) {
      it.value()->hoverRow = -1;
      static_cast<QWidget *>(watched)->update();
    }
  } else if (event->type() == QEvent::MouseButtonRelease) {
    auto *me = static_cast<QMouseEvent *>(event);
    if (me->button() == Qt::LeftButton) {
      auto checkListCancel =
          [&](QListWidget *list,
              const std::function<void(QListWidgetItem *)> &fn) -> bool {
        if (watched != list->viewport())
          return false;
        QListWidgetItem *item = list->itemAt(me->pos());
        if (!item || !item->data(ItemRole::ShowCancelAction).toBool())
          return false;
        const QRect rowRect = list->visualItemRect(item);
        if (PersonDelegate::cancelRect(rowRect).contains(me->pos())) {
          fn(item);
          return true;
        }
        return false;
      };
      if (checkListCancel(confirmedList_, [this](QListWidgetItem *item) {
            cancelBookingByIds(selectedEventId(),
                               item->data(kPersonAttendeeIdRole).toInt());
          }))
        return true;
      if (checkListCancel(waitlistList_, [this](QListWidgetItem *item) {
            cancelBookingByIds(selectedEventId(),
                               item->data(kPersonAttendeeIdRole).toInt());
          }))
        return true;
      if (checkListCancel(attendeeBookingsList_, [this](QListWidgetItem *item) {
            cancelBookingByIds(item->data(kBookingEventIdRole).toInt(),
                               selectedAttendeeId());
          }))
        return true;
    }
  }
  return QMainWindow::eventFilter(watched, event);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
  QMainWindow::resizeEvent(event);
  updateResponsiveLayout();
  if (toast_ && toast_->isVisible())
    toast_->reposition();
}

void MainWindow::showEvent(QShowEvent *event) {
  QMainWindow::showEvent(event);
  updateResponsiveLayout();
}

void MainWindow::updateResponsiveLayout() {
  if (!workspaceSplitter_ || !sidebarRail_ || !compactBar_)
    return;
  const int w = width();
  const int h = height();

  const bool collapseRail = (w < 1120);
  sidebarRail_->setVisible(!collapseRail);
  compactBar_->setVisible(collapseRail);
  if (!collapseRail) {
    const int railW = w < 1200 ? 196 : 216;
    if (sidebarRail_->width() != railW)
      sidebarRail_->setFixedWidth(railW);
    const bool showFooterToggles = (h >= 560);
    railAutoSaveBtn_->setVisible(showFooterToggles);
    railDarkBtn_->setVisible(showFooterToggles);
    railContrastBtn_->setVisible(showFooterToggles);
  }

  workspaceSplitter_->setOrientation(Qt::Horizontal);
  const int minInsp = collapseRail ? 300 : 340;
  eventInspectorPanel_->setMinimumWidth(minInsp);
  eventInspectorPanel_->setMaximumWidth(480);
  attendeeInspectorPanel_->setMinimumWidth(minInsp);
  attendeeInspectorPanel_->setMaximumWidth(480);
  updateTableColumns();
}

void MainWindow::updateTableColumns() {
  auto maxColContentWidth = [](const QTableWidget *t, int col, int cap = 420) {
    const QFontMetrics fm(t->font());
    const QString hdr = t->horizontalHeaderItem(col)
                            ? t->horizontalHeaderItem(col)->text()
                            : QString();
    int best = fm.horizontalAdvance(hdr) + 34;
    const int lim = qMin(t->rowCount(), 80);
    for (int r = 0; r < lim; ++r) {
      if (const auto *it = t->item(r, col))
        best = qMax(best, fm.horizontalAdvance(it->text()) + 30);
    }
    return qMin(best, cap);
  };

  if (eventsTable_) {
    int vw = eventsTable_->viewport()->width();
    if (vw <= 40)
      vw = qMax(420, eventsTable_->width());
    // Hiding priority: ID first, then Waitlist, then Venue; keep Time as long as possible.
    const bool hideId = vw < 780;
    const bool hideWaitlist = vw < 680;
    const bool hideVenue = vw < 540;
    const bool hideTime = vw < 430;

    eventsTable_->setColumnHidden(EventCol::Id, hideId);
    eventsTable_->setColumnHidden(EventCol::Waitlist, hideWaitlist);
    eventsTable_->setColumnHidden(EventCol::Venue, hideVenue);
    eventsTable_->setColumnHidden(EventCol::Time, hideTime);

    int bookedW = 140;
    {
      const QFontMetrics fm(eventsTable_->font());
      const int lim = qMin(eventsTable_->rowCount(), 80);
      int textW = fm.horizontalAdvance(QStringLiteral("0 / 100"));
      for (int r = 0; r < lim; ++r) {
        if (const auto *it = eventsTable_->item(r, EventCol::Booked)) {
          const int cap = it->data(ItemRole::Capacity).toInt();
          const int bkd = it->data(ItemRole::Booked).toInt();
          const QString txt = (cap > 0 && bkd >= cap)
                                  ? QStringLiteral("Full")
                                  : QStringLiteral("%1 / %2").arg(bkd).arg(cap);
          textW = qMax(textW, fm.horizontalAdvance(txt));
        }
      }
      // Match TableDelegate overhead: 2*Theme::cellPadding (24) + kMeterWidth (56) + kMeterGap (10).
      bookedW = qBound(140, 2 * Theme::cellPadding + 56 + 10 + textW + 6, 176);
    }

    eventsTable_->setColumnWidth(EventCol::Date, 102);
    eventsTable_->setColumnWidth(EventCol::Time, 80);
    eventsTable_->setColumnWidth(EventCol::Booked, bookedW);
    eventsTable_->setColumnWidth(EventCol::Waitlist, 66);
    eventsTable_->setColumnWidth(EventCol::Id, 56);

    auto *eh = eventsTable_->horizontalHeader();
    if (hideVenue) {
      eh->setSectionResizeMode(EventCol::Name, QHeaderView::Stretch);
    } else {
      const int fixedW = 102 + (hideTime ? 0 : 80) + bookedW +
                         (hideWaitlist ? 0 : 66) + (hideId ? 0 : 56);
      const int avail = qMax(220, vw - fixedW);
      const int naturalName = maxColContentWidth(eventsTable_, EventCol::Name);
      const int naturalVenue = maxColContentWidth(eventsTable_, EventCol::Venue);
      int nameW = naturalName;
      if (naturalName + naturalVenue > avail) {
        nameW = qBound(120, naturalName, avail * 56 / 100);
      } else {
        nameW = qBound(110, naturalName, avail - 90);
      }
      eh->setSectionResizeMode(EventCol::Name, QHeaderView::Fixed);
      eh->setSectionResizeMode(EventCol::Venue, QHeaderView::Stretch);
      eventsTable_->setColumnWidth(EventCol::Name, nameW);
    }
  }

  if (attendeesTable_) {
    int vw = attendeesTable_->viewport()->width();
    if (vw <= 40)
      vw = qMax(420, attendeesTable_->width());
    const bool hideId = vw < 620;
    const bool hideEmail = vw < 420;

    attendeesTable_->setColumnHidden(AttendeeCol::Id, hideId);
    attendeesTable_->setColumnHidden(AttendeeCol::Email, hideEmail);
    attendeesTable_->setColumnWidth(AttendeeCol::Bookings, 118);
    attendeesTable_->setColumnWidth(AttendeeCol::Id, 56);

    auto *ah = attendeesTable_->horizontalHeader();
    if (hideEmail) {
      ah->setSectionResizeMode(AttendeeCol::Name, QHeaderView::Stretch);
    } else {
      const int fixedW = 118 + (hideId ? 0 : 56);
      const int avail = qMax(220, vw - fixedW);
      const int naturalName = maxColContentWidth(attendeesTable_, AttendeeCol::Name);
      const int naturalEmail = maxColContentWidth(attendeesTable_, AttendeeCol::Email);
      int nameW = naturalName;
      if (naturalName + naturalEmail > avail) {
        nameW = qBound(120, naturalName, avail * 54 / 100);
      } else {
        nameW = qBound(110, naturalName, avail - 110);
      }
      ah->setSectionResizeMode(AttendeeCol::Name, QHeaderView::Fixed);
      ah->setSectionResizeMode(AttendeeCol::Email, QHeaderView::Stretch);
      attendeesTable_->setColumnWidth(AttendeeCol::Name, nameW);
    }
  }
}

void MainWindow::switchView(ViewMode mode) {
  activeView_ = mode;
  viewStack_->setCurrentIndex(int(mode));
  inspectorOuterStack_->setCurrentIndex(int(mode));
  updateSidebarState();
  updateActions();
  updateTableColumns();
}

void MainWindow::setStatusFilter(StatusFilter filter) {
  statusFilter_ = filter;
  updateSidebarState();
  filterEvents(eventSearch_->text());
}

void MainWindow::focusActiveSearch() {
  if (activeView_ == ViewMode::Events) {
    eventSearch_->setFocus();
    eventSearch_->selectAll();
  } else {
    attendeeSearch_->setFocus();
    attendeeSearch_->selectAll();
  }
}

MainWindow::StatusFilter MainWindow::eventStatus(const Event *e) const {
  const int cap = e->capacity();
  const int booked = int(e->confirmedCount());
  if (booked >= cap)
    return StatusFilter::Full;
  if (booked * 10 >= cap * 8)
    return StatusFilter::AlmostFull;
  return StatusFilter::Open;
}

void MainWindow::refreshAll() {
  const int eventId = selectedEventId();
  const int attendeeId = selectedAttendeeId();

    eventsTable_->blockSignals(true);
    eventsTable_->setSortingEnabled(false);
    eventsTable_->setRowCount(0);
    for (const Event* e : manager_.events()) {
        const int row = eventsTable_->rowCount();
        eventsTable_->insertRow(row);
        auto* nameCell = cell(toQ(e->name()));
        nameCell->setData(kRecordIdRole, e->id());
        nameCell->setToolTip(toQ(e->name()));
        auto* idCell = cell(QString::number(e->id()), true, true, true);
        idCell->setData(ItemRole::SortValue, qint64(e->id()));
        auto* dateCell = cell(toQ(e->date()), true, false, false);
        auto* timeCell = cell(toQ(e->time()), true, false, false);
        timeCell->setData(ItemRole::SortValue, qint64(DateUtil::timeToMinutes(e->time())));
        auto* venueCell = cell(toQ(e->venue()), true);
        venueCell->setToolTip(toQ(e->venue()));
        auto* booked = cell(QString("%1 / %2").arg(int(e->confirmedCount())).arg(e->capacity()), true);
        booked->setData(ItemRole::Kind, ItemRole::Seats);
        booked->setData(ItemRole::Booked, int(e->confirmedCount()));
        booked->setData(ItemRole::Capacity, e->capacity());
        booked->setData(ItemRole::SortValue, qint64(e->confirmedCount()) * 1000000LL + e->capacity());
        auto* waitCell = cell(QString::number(e->waitlistCount()), e->waitlistCount() == 0, true, false);
        waitCell->setData(ItemRole::SortValue, qint64(e->waitlistCount()));
        eventsTable_->setItem(row, EventCol::Name, nameCell);
        eventsTable_->setItem(row, EventCol::Date, dateCell);
        eventsTable_->setItem(row, EventCol::Time, timeCell);
        eventsTable_->setItem(row, EventCol::Venue, venueCell);
        eventsTable_->setItem(row, EventCol::Booked, booked);
        eventsTable_->setItem(row, EventCol::Waitlist, waitCell);
        eventsTable_->setItem(row, EventCol::Id, idCell);
    }
    markSharedPrefixElision(eventsTable_, EventCol::Name);
    markSharedPrefixElision(eventsTable_, EventCol::Venue);
    eventsTable_->setSortingEnabled(true);
    if ((eventId < 0 || !selectById(eventsTable_, eventId)) && eventsTable_->rowCount() > 0) eventsTable_->selectRow(0);
    eventsTable_->blockSignals(false);
    eventSearch_->setEnabled(eventsTable_->rowCount() > 0);
    filterEvents(eventSearch_->text());

  attendeesTable_->blockSignals(true);
  attendeesTable_->setSortingEnabled(false);
  attendeesTable_->setRowCount(0);
  const auto allEvents = manager_.events();
  for (const Attendee &a : manager_.attendees()) {
    int conf = 0;
    int wait = 0;
    for (const Event *e : allEvents) {
      if (e->isConfirmed(a.id))
        ++conf;
      else if (e->isWaitlisted(a.id))
        ++wait;
    }
    const QString summary =
        conf == 0 && wait == 0 ? QStringLiteral("None")
        : wait == 0 ? QString("%1 confirmed").arg(conf)
                    : QString("%1 conf · %2 wait").arg(conf).arg(wait);
    const int row = attendeesTable_->rowCount();
    attendeesTable_->insertRow(row);
    auto *nameCell = cell(toQ(a.name));
    nameCell->setData(kRecordIdRole, a.id);
    nameCell->setToolTip(toQ(a.name));
    auto *emailCell = cell(toQ(a.email), true);
    emailCell->setToolTip(toQ(a.email));
    auto *idCell = cell(QString::number(a.id), true, true, true);
    idCell->setData(ItemRole::SortValue, qint64(a.id));
    auto *bookCell = cell(summary, conf == 0 && wait == 0, true);
    bookCell->setData(ItemRole::SortValue, qint64(conf * 1000 + wait));
    attendeesTable_->setItem(row, AttendeeCol::Name, nameCell);
    attendeesTable_->setItem(row, AttendeeCol::Email, emailCell);
    attendeesTable_->setItem(row, AttendeeCol::Bookings, bookCell);
    attendeesTable_->setItem(row, AttendeeCol::Id, idCell);
  }
  markSharedPrefixElision(attendeesTable_, AttendeeCol::Name);
  markSharedPrefixElision(attendeesTable_, AttendeeCol::Email);
  attendeesTable_->setSortingEnabled(true);
  if ((attendeeId < 0 || !selectById(attendeesTable_, attendeeId)) &&
      attendeesTable_->rowCount() > 0)
    attendeesTable_->selectRow(0);
  attendeesTable_->blockSignals(false);
  attendeeSearch_->setEnabled(attendeesTable_->rowCount() > 0);
  filterAttendees(attendeeSearch_->text());

  refreshQuickBookCombos();
  refreshDetails();
  refreshAttendeeDetails();
  updateSidebarState();
  updateDegradedBanner();
  updateTableColumns();
  commands_.syncActions();
}

void MainWindow::refreshQuickBookCombos(bool preferUnbooked) {
  const int prevAttendee = quickBookAttendeeId();
  const Event *selEvent = manager_.findEvent(selectedEventId());
  quickBookCombo_->blockSignals(true);
  quickBookCombo_->clear();
  int firstUnbookedIdx = -1;
  for (const Attendee &a : manager_.attendees()) {
    const int row = quickBookCombo_->count();
    quickBookCombo_->addItem(QString("%1 (%2)").arg(toQ(a.name), toQ(a.email)),
                             a.id);
    if (firstUnbookedIdx < 0 && selEvent && !selEvent->isConfirmed(a.id) &&
        !selEvent->isWaitlisted(a.id))
      firstUnbookedIdx = row;
  }
  if (quickBookCombo_->count() == 0) {
    quickBookCombo_->setCurrentIndex(-1);
    quickBookCombo_->lineEdit()->clear();
  } else if (preferUnbooked && firstUnbookedIdx >= 0) {
    quickBookCombo_->setCurrentIndex(firstUnbookedIdx);
  } else if (preferUnbooked && firstUnbookedIdx < 0) {
    quickBookCombo_->setCurrentIndex(-1);
    quickBookCombo_->lineEdit()->clear();
  } else if (prevAttendee >= 0 &&
             quickBookCombo_->findData(prevAttendee) >= 0) {
    quickBookCombo_->setCurrentIndex(quickBookCombo_->findData(prevAttendee));
  } else if (firstUnbookedIdx >= 0) {
    quickBookCombo_->setCurrentIndex(firstUnbookedIdx);
  } else {
    quickBookCombo_->setCurrentIndex(-1);
    quickBookCombo_->lineEdit()->clear();
  }
  quickBookCombo_->blockSignals(false);
  if (quickBookCombo_->lineEdit() && !quickBookCombo_->lineEdit()->hasFocus())
    quickBookCombo_->lineEdit()->setCursorPosition(0);

  const int prevEvent = quickBookEventId();
  const Attendee *selAttendee = manager_.findAttendee(selectedAttendeeId());
  quickEventCombo_->blockSignals(true);
  quickEventCombo_->clear();
  int firstAvailableEv = -1;
  for (const Event *e : manager_.events()) {
    const int row = quickEventCombo_->count();
    quickEventCombo_->addItem(
        QString("%1 (%2)").arg(toQ(e->name()), formatDisplayDate(e->date())),
        e->id());
    if (firstAvailableEv < 0 && selAttendee &&
        !e->isConfirmed(selAttendee->id) && !e->isWaitlisted(selAttendee->id))
      firstAvailableEv = row;
  }
  if (quickEventCombo_->count() == 0) {
    quickEventCombo_->setCurrentIndex(-1);
    quickEventCombo_->lineEdit()->clear();
  } else if (preferUnbooked && firstAvailableEv >= 0) {
    quickEventCombo_->setCurrentIndex(firstAvailableEv);
  } else if (preferUnbooked && firstAvailableEv < 0) {
    quickEventCombo_->setCurrentIndex(-1);
    quickEventCombo_->lineEdit()->clear();
  } else if (prevEvent >= 0 && quickEventCombo_->findData(prevEvent) >= 0) {
    quickEventCombo_->setCurrentIndex(quickEventCombo_->findData(prevEvent));
  } else if (firstAvailableEv >= 0) {
    quickEventCombo_->setCurrentIndex(firstAvailableEv);
  } else {
    quickEventCombo_->setCurrentIndex(-1);
    quickEventCombo_->lineEdit()->clear();
  }
  quickEventCombo_->blockSignals(false);
  if (quickEventCombo_->lineEdit() && !quickEventCombo_->lineEdit()->hasFocus())
    quickEventCombo_->lineEdit()->setCursorPosition(0);
}

void MainWindow::refreshDetails() {
  confirmedList_->clear();
  waitlistList_->clear();

  const Event *e = manager_.findEvent(selectedEventId());
  if (!e) {
    eventInspectorStack_->setCurrentIndex(0);
    updateActions();
    return;
  }
  eventInspectorStack_->setCurrentIndex(1);

  eventTitle_->setText(toQ(e->name()));
  const QString dateText = formatDisplayDate(e->date());
  eventMeta_->setText(QString("%1  \xC2\xB7  %2  \xC2\xB7  %3")
                          .arg(dateText, toQ(e->time()), toQ(e->venue())));
  const QString desc = toQ(e->description()).trimmed();
  eventDescription_->setText(desc);
  eventDescription_->setVisible(!desc.isEmpty());

  const int capacity = e->capacity();
  const int booked = int(e->confirmedCount());
  const StatusFilter status = eventStatus(e);
  seatsLabel_->setText(
      QString("%1 of %2 seats booked").arg(booked).arg(capacity));
  seatMeter_->setRange(0, capacity);
  seatMeter_->setValue(booked);
  seatMeter_->setProperty("state", status == StatusFilter::Full         ? "full"
                                   : status == StatusFilter::AlmostFull ? "warn"
                                                                        : "ok");
  Theme::repolish(seatMeter_);
  if (status == StatusFilter::Full)
    setChip(statusChip_, "danger", "Full");
  else if (status == StatusFilter::AlmostFull)
    setChip(statusChip_, "warning", "Almost Full");
  else
    setChip(statusChip_, "success", "Open");

  std::vector<int> confirmed = e->confirmedIds();
  std::sort(confirmed.begin(), confirmed.end());
  for (int id : confirmed) {
    const Attendee *a = manager_.findAttendee(id);
    auto *item =
        new QListWidgetItem(a ? toQ(a->name) : QString("Attendee #%1").arg(id));
    item->setData(ItemRole::Email, a ? toQ(a->email) : QString());
    item->setData(ItemRole::ShowCancelAction, true);
    item->setData(kPersonAttendeeIdRole, id);
    confirmedList_->addItem(item);
  }
  int position = 1;
  for (int id : e->waitlistIds()) {
    const Attendee *a = manager_.findAttendee(id);
    auto *item =
        new QListWidgetItem(a ? toQ(a->name) : QString("Attendee #%1").arg(id));
    item->setData(ItemRole::Email, a ? toQ(a->email) : QString());
    item->setData(ItemRole::Position, position++);
    item->setData(ItemRole::ShowCancelAction, true);
    item->setData(kPersonAttendeeIdRole, id);
    waitlistList_->addItem(item);
  }
  const int confRows = confirmedList_->count();
  const int waitRows = waitlistList_->count();
  confirmedCount_->setText(QString::number(confRows));
  waitlistCount_->setText(QString::number(waitRows));
  confirmedStack_->setCurrentIndex(confRows > 0 ? 1 : 0);
  waitlistStack_->setCurrentIndex(waitRows > 0 ? 1 : 0);

  // Size empty-state stacks from wrapped sizeHint so multi-line helper copy
  // never clips vertically, while avoiding dead vertical void.
  auto emptyStackHeight = [](QStackedWidget *stack) {
    if (QWidget *w = stack->widget(0)) {
      const int wHint = qMax(240, stack->width() > 40 ? stack->width() : 320);
      if (w->layout())
        w->layout()->activate();
      const int hfw = w->hasHeightForWidth() ? w->heightForWidth(wHint) : -1;
      return qMax(76, qMax(hfw, w->sizeHint().height()) + 4);
    }
    return 84;
  };
  const int emptyConfH = emptyStackHeight(confirmedStack_);
  const int emptyWaitH = emptyStackHeight(waitlistStack_);

  if (waitRows == 0) {
    waitlistStack_->setMinimumHeight(emptyWaitH);
    waitlistStack_->setMaximumHeight(emptyWaitH);
    eventDetailLayout_->setStretchFactor(waitlistStack_, 0);

    if (confRows <= 5) {
      const int h = confRows == 0 ? emptyConfH : confRows * Theme::rowHeight + 4;
      confirmedStack_->setMinimumHeight(h);
      confirmedStack_->setMaximumHeight(h);
      eventDetailLayout_->setStretchFactor(confirmedStack_, 0);
      detailTailSpacer_->show();
      eventDetailLayout_->setStretchFactor(detailTailSpacer_, 1);
    } else {
      confirmedStack_->setMinimumHeight(Theme::rowHeight * 3);
      confirmedStack_->setMaximumHeight(QWIDGETSIZE_MAX);
      eventDetailLayout_->setStretchFactor(confirmedStack_, 1);
      detailTailSpacer_->hide();
      eventDetailLayout_->setStretchFactor(detailTailSpacer_, 0);
    }
  } else {
    waitlistStack_->setMinimumHeight(Theme::rowHeight * 2);
    waitlistStack_->setMaximumHeight(QWIDGETSIZE_MAX);
    if (confRows <= 3) {
      const int h = confRows == 0 ? emptyConfH : confRows * Theme::rowHeight + 4;
      confirmedStack_->setMinimumHeight(h);
      confirmedStack_->setMaximumHeight(h);
      eventDetailLayout_->setStretchFactor(confirmedStack_, 0);
    } else {
      confirmedStack_->setMinimumHeight(Theme::rowHeight * 2);
      confirmedStack_->setMaximumHeight(QWIDGETSIZE_MAX);
      eventDetailLayout_->setStretchFactor(confirmedStack_, 1);
    }
    eventDetailLayout_->setStretchFactor(waitlistStack_, 1);
    detailTailSpacer_->hide();
    eventDetailLayout_->setStretchFactor(detailTailSpacer_, 0);
  }

  refreshQuickBookCombos(true);
  updateActions();
}

void MainWindow::refreshAttendeeDetails() {
  attendeeBookingsList_->clear();
  const Attendee *a = manager_.findAttendee(selectedAttendeeId());
  if (!a) {
    attendeeInspectorStack_->setCurrentIndex(0);
    updateActions();
    return;
  }
  attendeeInspectorStack_->setCurrentIndex(1);

  attendeeTitle_->setText(toQ(a->name));
  attendeeEmail_->setText(toQ(a->email));

  int conf = 0;
  int wait = 0;
  for (const Event *e : manager_.events()) {
    if (e->isConfirmed(a->id)) {
      ++conf;
      auto *item = new QListWidgetItem(toQ(e->name()));
      item->setData(
          ItemRole::Email,
          QString("%1 · %2").arg(formatDisplayDate(e->date()), toQ(e->time())));
      item->setData(ItemRole::BadgeText, QStringLiteral("Confirmed"));
      item->setData(ItemRole::BadgeKind, QStringLiteral("success"));
      item->setData(ItemRole::ShowCancelAction, true);
      item->setData(kBookingEventIdRole, e->id());
      attendeeBookingsList_->addItem(item);
    } else if (e->isWaitlisted(a->id)) {
      ++wait;
      int pos = 1;
      for (int wid : e->waitlistIds()) {
        if (wid == a->id)
          break;
        ++pos;
      }
      auto *item = new QListWidgetItem(toQ(e->name()));
      item->setData(
          ItemRole::Email,
          QString("%1 · %2").arg(formatDisplayDate(e->date()), toQ(e->time())));
      item->setData(ItemRole::BadgeText, QString("Waitlist #%1").arg(pos));
      item->setData(ItemRole::BadgeKind, QStringLiteral("warning"));
      item->setData(ItemRole::ShowCancelAction, true);
      item->setData(kBookingEventIdRole, e->id());
      attendeeBookingsList_->addItem(item);
    }
  }

  attendeeSummary_->setText(
      QString("%1 confirmed  ·  %2 waitlisted").arg(conf).arg(wait));
  const int bookingsRows = attendeeBookingsList_->count();
  attendeeBookingsCount_->setText(QString::number(bookingsRows));
  attendeeBookingsStack_->setCurrentIndex(bookingsRows > 0 ? 1 : 0);

  if (bookingsRows == 0) {
    int emptyH = 76;
    if (QWidget *w = attendeeBookingsStack_->widget(0)) {
      if (w->layout())
        w->layout()->activate();
      emptyH = qMax(76, w->sizeHint().height() + 4);
    }
    attendeeBookingsStack_->setMinimumHeight(emptyH);
    attendeeBookingsStack_->setMaximumHeight(emptyH);
    attendeeDetailLayout_->setStretchFactor(attendeeBookingsStack_, 0);
    attendeeTailSpacer_->show();
    attendeeDetailLayout_->setStretchFactor(attendeeTailSpacer_, 1);
  } else {
    attendeeBookingsStack_->setMinimumHeight(Theme::rowHeight * 2);
    attendeeBookingsStack_->setMaximumHeight(QWIDGETSIZE_MAX);
    attendeeDetailLayout_->setStretchFactor(attendeeBookingsStack_, 1);
    attendeeTailSpacer_->hide();
    attendeeDetailLayout_->setStretchFactor(attendeeTailSpacer_, 0);
  }

  refreshQuickBookCombos(true);
  updateActions();
}

void MainWindow::updateActions() {
  const Event *e = manager_.findEvent(selectedEventId());
  editEventBtn_->setEnabled(e != nullptr);
  deleteEventBtn_->setEnabled(e != nullptr);

  const int aid = quickBookAttendeeId();
  const Attendee *qa = manager_.findAttendee(aid);
  if (!e) {
    quickBookStateChip_->hide();
    bookButton_->hide();
    cancelButton_->hide();
    actionHint_->setText("Select an event to book attendees.");
  } else if (manager_.attendees().empty()) {
    quickBookStateChip_->hide();
    bookButton_->hide();
    cancelButton_->hide();
    actionHint_->setText(
        "Register an attendee first (Ctrl+Shift+N) to take bookings.");
  } else {
    bool allBooked = true;
    for (const Attendee &a : manager_.attendees()) {
      if (!e->isConfirmed(a.id) && !e->isWaitlisted(a.id)) {
        allBooked = false;
        break;
      }
    }
    if (!qa) {
      quickBookStateChip_->hide();
      cancelButton_->hide();
      if (allBooked) {
        bookButton_->hide();
        actionHint_->setText("All attendees are booked for this event.");
      } else {
        bookButton_->setText(e->seatsAvailable() <= 0
                                 ? QStringLiteral("Join Waitlist")
                                 : QStringLiteral("Book Attendee"));
        bookButton_->show();
        bookButton_->setEnabled(true);
        actionHint_->setText("Select an attendee above to book.");
      }
    } else {
      const bool confirmed = e->isConfirmed(qa->id);
      const bool waitlisted = e->isWaitlisted(qa->id);
      if (confirmed) {
        setChip(quickBookStateChip_, "success", "Booked");
        quickBookStateChip_->show();
        actionHint_->setText(
            QString("%1 is confirmed for this event.").arg(toQ(qa->name)));
        bookButton_->hide();
        cancelButton_->show();
        cancelButton_->setEnabled(true);
      } else if (waitlisted) {
        int pos = 1;
        for (int wid : e->waitlistIds()) {
          if (wid == qa->id)
            break;
          ++pos;
        }
        setChip(quickBookStateChip_, "warning",
                QString("Waitlisted #%1").arg(pos));
        quickBookStateChip_->show();
        actionHint_->setText(QString("%1 is on the waitlist at position %2.")
                                 .arg(toQ(qa->name))
                                 .arg(pos));
        bookButton_->hide();
        cancelButton_->show();
        cancelButton_->setEnabled(true);
      } else {
        quickBookStateChip_->hide();
        cancelButton_->hide();
        bookButton_->setText(e->seatsAvailable() <= 0
                                 ? QStringLiteral("Join Waitlist")
                                 : QStringLiteral("Book Attendee"));
        bookButton_->show();
        bookButton_->setEnabled(true);
        actionHint_->setText(
            e->seatsAvailable() <= 0
                ? "Event is full — booking adds them to the waitlist."
                : "Ready to book into an open seat.");
      }
    }
  }

  const Attendee *sa = manager_.findAttendee(selectedAttendeeId());
  editAttendeeBtn_->setEnabled(sa != nullptr);
  deleteAttendeeBtn_->setEnabled(sa != nullptr);
  const int qeId = quickBookEventId();
  const Event *qe = manager_.findEvent(qeId);
  if (!sa) {
    attendeeBookStateChip_->hide();
    bookEventBtn_->hide();
    cancelEventBookingBtn_->hide();
    attendeeActionHint_->setText("Select an attendee to manage bookings.");
  } else if (manager_.events().empty()) {
    attendeeBookStateChip_->hide();
    bookEventBtn_->hide();
    cancelEventBookingBtn_->hide();
    attendeeActionHint_->setText(
        "Create an event first (Ctrl+N) to take bookings.");
  } else {
    bool allEventsBooked = true;
    for (const Event *ev : manager_.events()) {
      if (!ev->isConfirmed(sa->id) && !ev->isWaitlisted(sa->id)) {
        allEventsBooked = false;
        break;
      }
    }
    if (!qe) {
      attendeeBookStateChip_->hide();
      cancelEventBookingBtn_->hide();
      if (allEventsBooked) {
        bookEventBtn_->hide();
        attendeeActionHint_->setText("Booked into all events.");
      } else {
        bookEventBtn_->setText("Book Event");
        bookEventBtn_->show();
        bookEventBtn_->setEnabled(true);
        attendeeActionHint_->setText("Select an event above to book.");
      }
    } else {
      const bool confirmed = qe->isConfirmed(sa->id);
      const bool waitlisted = qe->isWaitlisted(sa->id);
      if (confirmed) {
        setChip(attendeeBookStateChip_, "success", "Booked");
        attendeeBookStateChip_->show();
        attendeeActionHint_->setText(
            QString("Confirmed for %1.").arg(toQ(qe->name())));
        bookEventBtn_->hide();
        cancelEventBookingBtn_->show();
        cancelEventBookingBtn_->setEnabled(true);
      } else if (waitlisted) {
        int pos = 1;
        for (int wid : qe->waitlistIds()) {
          if (wid == sa->id)
            break;
          ++pos;
        }
        setChip(attendeeBookStateChip_, "warning",
                QString("Waitlisted #%1").arg(pos));
        attendeeBookStateChip_->show();
        attendeeActionHint_->setText(
            QString("Waitlisted at position %1 for %2.")
                .arg(pos)
                .arg(toQ(qe->name())));
        bookEventBtn_->hide();
        cancelEventBookingBtn_->show();
        cancelEventBookingBtn_->setEnabled(true);
      } else {
        attendeeBookStateChip_->hide();
        cancelEventBookingBtn_->hide();
        bookEventBtn_->setText(qe->seatsAvailable() <= 0
                                   ? QStringLiteral("Join Waitlist")
                                   : QStringLiteral("Book Event"));
        bookEventBtn_->show();
        bookEventBtn_->setEnabled(true);
        attendeeActionHint_->setText(
            qe->seatsAvailable() <= 0
                ? "Event is full — booking adds them to the waitlist."
                : "Ready to book into an open seat.");
      }
    }
  }
  commands_.syncActions();
}

void MainWindow::updateSidebarState() {
  navEventsBtn_->setProperty("active", activeView_ == ViewMode::Events);
  navAttendeesBtn_->setProperty("active", activeView_ == ViewMode::Attendees);
  Theme::repolish(navEventsBtn_);
  Theme::repolish(navAttendeesBtn_);

  if (compactEventsBtn_ && compactAttendeesBtn_) {
    compactEventsBtn_->setProperty("active", activeView_ == ViewMode::Events);
    compactAttendeesBtn_->setProperty("active",
                                      activeView_ == ViewMode::Attendees);
    Theme::repolish(compactEventsBtn_);
    Theme::repolish(compactAttendeesBtn_);
  }

  navEventsBadge_->setText(QString::number(eventsTable_->rowCount()));
  navAttendeesBadge_->setText(QString::number(attendeesTable_->rowCount()));
  navEventsBadge_->setVisible(!degradedMode_);
  navAttendeesBadge_->setVisible(!degradedMode_);
  eventsCount_->setVisible(!degradedMode_);
  attendeesCount_->setVisible(!degradedMode_);

  eventsSegFrame_->setVisible(!degradedMode_);
  eventsNewBtn_->setVisible(!degradedMode_);
  attendeesRegisterBtn_->setVisible(!degradedMode_);
  eventSearch_->setVisible(!degradedMode_);
  attendeeSearch_->setVisible(!degradedMode_);

  for (int i = 0; i < 4; ++i) {
    segFilterButtons_[i]->blockSignals(true);
    segFilterButtons_[i]->setChecked(int(statusFilter_) == i);
    segFilterButtons_[i]->blockSignals(false);
  }

  railAutoSaveBtn_->blockSignals(true);
  railAutoSaveBtn_->setChecked(autoSave_);
  railAutoSaveBtn_->setAccessibleDescription(
      degradedMode_ ? QStringLiteral("Paused in recovery mode")
      : autoSave_   ? QStringLiteral("On")
                    : QStringLiteral("Off"));
  railAutoSaveBtn_->blockSignals(false);
  railDarkBtn_->blockSignals(true);
  railDarkBtn_->setChecked(Theme::mode() == Theme::Mode::Dark);
  railDarkBtn_->blockSignals(false);
  railContrastBtn_->blockSignals(true);
  railContrastBtn_->setChecked(Theme::highContrast());
  railContrastBtn_->blockSignals(false);

  setChip(railAutoSaveBadge_,
          autoSave_ && !degradedMode_ ? "subtle" : "neutral",
          degradedMode_ ? "Paused"
          : autoSave_   ? "On"
                        : "Off");
  setChip(railDarkBadge_,
          Theme::mode() == Theme::Mode::Dark ? "subtle" : "neutral",
          Theme::mode() == Theme::Mode::Dark ? "On" : "Off");
  setChip(railContrastBadge_, Theme::highContrast() ? "subtle" : "neutral",
          Theme::highContrast() ? "On" : "Off");

  const char *dotRole = degradedMode_ ? "danger"
                        : dirty_      ? "warning"
                                      : "success";
  railStatusDot_->setProperty("statusDot", dotRole);
  Theme::repolish(railStatusDot_);
  if (compactStatusDot_) {
    compactStatusDot_->setProperty("statusDot", dotRole);
    Theme::repolish(compactStatusDot_);
  }
  railStatusText_->setText(degradedMode_ ? "Recovery mode"
                           : dirty_      ? "Unsaved changes"
                                         : "All changes saved");
}

void MainWindow::updateDegradedBanner() {
  degradedBanner_->setVisible(degradedMode_);
  if (!degradedMode_)
    return;
  degradedLabel_->setText(
      QString("Data file problem (%1) — auto-save is paused so %2 won't be "
              "overwritten.")
          .arg(degradedReason_, QFileInfo(dataPath_).fileName()));
  const bool hasBackup = Storage::hasBackup(dataPath_.toStdString());
  restoreBackupBtn_->setEnabled(true);
  restoreBackupBtn_->setProperty("inactive", !hasBackup);
  restoreBackupBtn_->setToolTip(
      hasBackup ? QStringLiteral("Restore from the last good .bak backup")
                : QStringLiteral("Unavailable: no .bak backup file exists yet"));
  restoreBackupBtn_->setAccessibleDescription(
      hasBackup ? QStringLiteral("Restores the database from its .bak backup file")
                : QStringLiteral("No backup file exists yet for this database"));
  Theme::repolish(restoreBackupBtn_);
}

void MainWindow::enterDegradedMode(const QString &reason) {
  degradedMode_ = true;
  degradedReason_ = reason;
  updateDegradedBanner();
  updateSidebarState();
}

void MainWindow::clearDegradedMode() {
  degradedMode_ = false;
  degradedReason_.clear();
  updateDegradedBanner();
  updateSidebarState();
}

QString MainWindow::attendeeName(int attendeeId) const {
  const Attendee *a = manager_.findAttendee(attendeeId);
  return a ? toQ(a->name) : QString("Attendee #%1").arg(attendeeId);
}

int MainWindow::selectedEventId() const { return selectedId(eventsTable_); }
int MainWindow::selectedAttendeeId() const {
  return selectedId(attendeesTable_);
}

int MainWindow::quickBookAttendeeId() const {
  const QString text = quickBookCombo_->currentText();
  int idx = quickBookCombo_->currentIndex();
  if (idx < 0 || text != quickBookCombo_->itemText(idx))
    idx = quickBookCombo_->findText(text, Qt::MatchExactly);
  return idx >= 0 ? quickBookCombo_->itemData(idx).toInt() : -1;
}

int MainWindow::quickBookEventId() const {
  const QString text = quickEventCombo_->currentText();
  int idx = quickEventCombo_->currentIndex();
  if (idx < 0 || text != quickEventCombo_->itemText(idx))
    idx = quickEventCombo_->findText(text, Qt::MatchExactly);
  return idx >= 0 ? quickEventCombo_->itemData(idx).toInt() : -1;
}

void MainWindow::addEvent() {
  EventDialog dialog(this);
  while (dialog.exec() == QDialog::Accepted) {
    try {
      undoStack_.push(manager_);
      const int id = manager_.createEvent(
          dialog.name().toStdString(), dialog.date().toStdString(),
          dialog.venue().toStdString(), dialog.capacity(),
          dialog.description().toStdString(), dialog.time().toStdString());
      markDirty(QString("%1 created").arg(dialog.name()));
      refreshAll();
      if (activeView_ == ViewMode::Events)
        selectById(eventsTable_, id);
      return;
    } catch (const std::exception &ex) {
      undoStack_.discardLast();
      dialog.showNameError(friendlyError(ex));
    }
  }
}

void MainWindow::editEvent() {
  const Event *e = manager_.findEvent(selectedEventId());
  if (!e)
    return;
  const int id = e->id();
  EventDialog dialog(toQ(e->name()), toQ(e->date()), toQ(e->time()),
                     toQ(e->venue()), e->capacity(), toQ(e->description()),
                     this);
  while (dialog.exec() == QDialog::Accepted) {
    try {
      undoStack_.push(manager_);
      const auto promoted = manager_.updateEvent(
          id, dialog.name().toStdString(), dialog.date().toStdString(),
          dialog.venue().toStdString(), dialog.capacity(),
          dialog.description().toStdString(), dialog.time().toStdString());
      QString msg = QString("%1 updated").arg(dialog.name());
      if (!promoted.empty())
        msg += QString(" · %1 promoted from waitlist").arg(promoted.size());
      markDirty(msg);
      refreshAll();
      selectById(eventsTable_, id);
      return;
    } catch (const std::exception &ex) {
      undoStack_.discardLast();
      dialog.showNameError(friendlyError(ex));
    }
  }
}


void MainWindow::deleteEvent() {
  const Event *e = manager_.findEvent(selectedEventId());
  if (!e)
    return;
  const int id = e->id();
  const QString name = toQ(e->name());
  const int bookings = int(e->confirmedCount());
  const int waiting = int(e->waitlistCount());

  const QString body = QString("Removes %1 confirmed %2 and %3 waitlist %4. "
                               "You can undo this right away.")
                           .arg(bookings)
                           .arg(bookings == 1 ? "booking" : "bookings")
                           .arg(waiting)
                           .arg(waiting == 1 ? "entry" : "entries");
  if (!ConfirmDialog::ask(this, QString("Delete %1?").arg(Theme::quoted(name)),
                          body, "Delete Event", "Keep Event", true)) {
    return;
  }

  undoStack_.push(manager_);
  manager_.deleteEvent(id);
  markDirty(QString("Event %1 deleted").arg(Theme::quoted(name)));
  refreshAll();
}

void MainWindow::registerAttendee() {
  AttendeeDialog dialog(this);
  while (dialog.exec() == QDialog::Accepted) {
    try {
      undoStack_.push(manager_);
      const int id = manager_.registerAttendee(dialog.name().toStdString(),
                                               dialog.email().toStdString());
      markDirty(QString("%1 registered").arg(dialog.name()));
      refreshAll();
      if (activeView_ == ViewMode::Attendees)
        selectById(attendeesTable_, id);
      const int idx = quickBookCombo_->findData(id);
      if (idx >= 0)
        quickBookCombo_->setCurrentIndex(idx);
      return;
    } catch (const std::exception &ex) {
      undoStack_.discardLast();
      dialog.showEmailError(friendlyError(ex));
    }
  }
}

void MainWindow::editAttendee() {
  const Attendee *a = manager_.findAttendee(selectedAttendeeId());
  if (!a)
    return;
  const int id = a->id;
  AttendeeDialog dialog(toQ(a->name), toQ(a->email), this);
  while (dialog.exec() == QDialog::Accepted) {
    try {
      undoStack_.push(manager_);
      manager_.updateAttendee(id, dialog.name().toStdString(),
                              dialog.email().toStdString());
      markDirty(QString("%1 updated").arg(dialog.name()));
      refreshAll();
      selectById(attendeesTable_, id);
      return;
    } catch (const std::exception &ex) {
      undoStack_.discardLast();
      dialog.showEmailError(friendlyError(ex));
    }
  }
}

void MainWindow::deleteAttendee() {
  const Attendee *a = manager_.findAttendee(selectedAttendeeId());
  if (!a)
    return;
  const int id = a->id;
  const QString name = toQ(a->name);

  int confirmed = 0;
  int waitlisted = 0;
  for (const Event *e : manager_.events()) {
    if (e->isConfirmed(id))
      ++confirmed;
    else if (e->isWaitlisted(id))
      ++waitlisted;
  }
  const QString body =
      (confirmed == 0 && waitlisted == 0)
          ? QStringLiteral("No active bookings or waitlist entries. You can "
                           "undo this right away.")
          : QString("Removes %1 confirmed %2 and %3 waitlist %4. You can undo "
                    "this right away.")
                .arg(confirmed)
                .arg(confirmed == 1 ? "booking" : "bookings")
                .arg(waitlisted)
                .arg(waitlisted == 1 ? "entry" : "entries");

  if (!ConfirmDialog::ask(this, QString("Delete %1?").arg(Theme::quoted(name)),
                          body, "Delete Attendee", "Keep Attendee", true)) {
    return;
  }

  undoStack_.push(manager_);
  manager_.deleteAttendee(id);
  markDirty(QString("%1 deleted").arg(name));
  refreshAll();
}

void MainWindow::bookFromEventInspector() {
  const int eventId = selectedEventId();
  const int attendeeId = quickBookAttendeeId();
  if (eventId < 0)
    return;
  if (attendeeId < 0) {
    actionHint_->setText("Select a registered attendee from the dropdown first.");
    toast_->showMessage("Select a registered attendee above before booking");
    quickBookCombo_->setFocus();
    return;
  }
  try {
    const Event *e = manager_.findEvent(eventId);
    const QString who = attendeeName(attendeeId);
    const QString what = e ? toQ(e->name()) : QString();
    undoStack_.push(manager_);
    const Event::BookResult res = manager_.book(eventId, attendeeId);
    if (res == Event::BookResult::Duplicate) {
      undoStack_.discardLast();
      toast_->showMessage(
          QString("%1 is already booked for %2").arg(who, Theme::quoted(what)));
      return;
    }
    const QString msg =
        res == Event::BookResult::Confirmed
            ? QString("%1 booked into %2").arg(who, Theme::quoted(what))
            : QString("%1 added to waitlist for %2")
                  .arg(who, Theme::quoted(what));
    markDirty(msg);
    refreshAll();
    refreshQuickBookCombos(true);
    updateActions();
  } catch (const std::exception &ex) {
    undoStack_.discardLast();
    showError("Couldn't Book Attendee", friendlyError(ex));
  }
}

void MainWindow::bookFromAttendeeInspector() {
  const int attendeeId = selectedAttendeeId();
  const int eventId = quickBookEventId();
  if (attendeeId < 0)
    return;
  if (eventId < 0) {
    attendeeActionHint_->setText("Select an event from the dropdown first.");
    toast_->showMessage("Select an event above before booking");
    quickEventCombo_->setFocus();
    return;
  }
  try {
    const Event *e = manager_.findEvent(eventId);
    const QString who = attendeeName(attendeeId);
    const QString what = e ? toQ(e->name()) : QString();
    undoStack_.push(manager_);
    const Event::BookResult res = manager_.book(eventId, attendeeId);
    if (res == Event::BookResult::Duplicate) {
      undoStack_.discardLast();
      toast_->showMessage(
          QString("%1 is already booked for %2").arg(who, Theme::quoted(what)));
      return;
    }
    const QString msg =
        res == Event::BookResult::Confirmed
            ? QString("%1 booked into %2").arg(who, Theme::quoted(what))
            : QString("%1 added to waitlist for %2")
                  .arg(who, Theme::quoted(what));
    markDirty(msg);
    refreshAll();
    refreshQuickBookCombos(true);
    updateActions();
  } catch (const std::exception &ex) {
    undoStack_.discardLast();
    showError("Couldn't Book Event", friendlyError(ex));
  }
}

void MainWindow::cancelSelectedBooking() {
  cancelBookingByIds(selectedEventId(), quickBookAttendeeId());
}

void MainWindow::cancelAttendeeQuickBooking() {
  cancelBookingByIds(quickBookEventId(), selectedAttendeeId());
}

void MainWindow::cancelBookingByIds(int eventId, int attendeeId) {
  if (eventId < 0 || attendeeId < 0)
    return;
  try {
    const QString who = attendeeName(attendeeId);
    undoStack_.push(manager_);
    const Event::CancelResult result = manager_.cancel(eventId, attendeeId);
    if (!result.cancelled) {
      undoStack_.discardLast();
      toast_->showMessage(QString("%1 has no booking for this event").arg(who));
      return;
    }
    QString message = QString("Booking cancelled for %1").arg(who);
    if (result.promotedAttendeeId)
      message += QString(" · %1 promoted from waitlist")
                     .arg(attendeeName(*result.promotedAttendeeId));
    markDirty(message);
    refreshAll();
  } catch (const std::exception &ex) {
    undoStack_.discardLast();
    showError("Couldn't Cancel Booking", friendlyError(ex));
  }
}

void MainWindow::jumpToEvent(int eventId) {
  setStatusFilter(StatusFilter::All);
  eventSearch_->clear();
  switchView(ViewMode::Events);
  selectById(eventsTable_, eventId);
}

void MainWindow::jumpToAttendee(int attendeeId) {
  attendeeSearch_->clear();
  switchView(ViewMode::Attendees);
  selectById(attendeesTable_, attendeeId);
}

void MainWindow::filterEvents(const QString &query) {
  const QString needle = query.trimmed().toLower();
  const int total = eventsTable_->rowCount();
  int firstVisible = -1;
  int visibleCount = 0;

  for (int row = 0; row < total; ++row) {
    const int id = eventsTable_->item(row, 0)->data(kRecordIdRole).toInt();
    const Event *ev = manager_.findEvent(id);
    const bool statusMatch = statusFilter_ == StatusFilter::All ||
                             (ev && eventStatus(ev) == statusFilter_);
    const bool textMatch =
        needle.isEmpty() ||
        eventsTable_->item(row, EventCol::Name)->text().toLower().contains(needle) ||
        eventsTable_->item(row, EventCol::Date)->text().toLower().contains(needle) ||
        eventsTable_->item(row, EventCol::Time)->text().toLower().contains(needle) ||
        eventsTable_->item(row, EventCol::Venue)->text().toLower().contains(needle) ||
        eventsTable_->item(row, EventCol::Id)->text().toLower().contains(needle);
    const bool show = statusMatch && textMatch;
    eventsTable_->setRowHidden(row, !show);
    if (show) {
      ++visibleCount;
      if (firstVisible < 0)
        firstVisible = row;
    }
  }

  const int cur = eventsTable_->currentRow();
  if (cur < 0 || eventsTable_->isRowHidden(cur)) {
    if (firstVisible >= 0)
      eventsTable_->selectRow(firstVisible);
    else
      eventsTable_->clearSelection();
  }

  if (total == 0) {
    eventsStack_->setCurrentIndex(degradedMode_ ? 2 : 0);
    eventsCount_->setText("0");
  } else if (visibleCount == 0) {
    eventsFilterEmptyLabel_->setText(
        needle.isEmpty()
            ? QStringLiteral("No events match the selected status filter.")
            : QString("No events match “%1”.").arg(query.trimmed()));
    eventsStack_->setCurrentIndex(1);
    eventsCount_->setText(QString("0 / %1").arg(total));
  } else {
    eventsStack_->setCurrentIndex(3);
    eventsCount_->setText(
        visibleCount == total
            ? QString::number(total)
            : QString("%1 / %2").arg(visibleCount).arg(total));
  }
  eventsCount_->setVisible(!degradedMode_);
  refreshDetails();
}

void MainWindow::filterAttendees(const QString &query) {
  const QString needle = query.trimmed().toLower();
  const int total = attendeesTable_->rowCount();
  int firstVisible = -1;
  int visibleCount = 0;

  for (int row = 0; row < total; ++row) {
    const bool match =
        needle.isEmpty() ||
        attendeesTable_->item(row, AttendeeCol::Name)->text().toLower().contains(needle) ||
        attendeesTable_->item(row, AttendeeCol::Email)->text().toLower().contains(needle) ||
        attendeesTable_->item(row, AttendeeCol::Id)->text().toLower().contains(needle);
    attendeesTable_->setRowHidden(row, !match);
    if (match) {
      ++visibleCount;
      if (firstVisible < 0)
        firstVisible = row;
    }
  }

  const int cur = attendeesTable_->currentRow();
  if (cur < 0 || attendeesTable_->isRowHidden(cur)) {
    if (firstVisible >= 0)
      attendeesTable_->selectRow(firstVisible);
    else
      attendeesTable_->clearSelection();
  }

  if (total == 0) {
    attendeesStack_->setCurrentIndex(degradedMode_ ? 2 : 0);
    attendeesCount_->setText("0");
  } else if (visibleCount == 0) {
    attendeesFilterEmptyLabel_->setText(
        QString("No attendees match “%1”.").arg(query.trimmed()));
    attendeesStack_->setCurrentIndex(1);
    attendeesCount_->setText(QString("0 / %1").arg(total));
  } else {
    attendeesStack_->setCurrentIndex(3);
    attendeesCount_->setText(
        visibleCount == total
            ? QString::number(total)
            : QString("%1 / %2").arg(visibleCount).arg(total));
  }
  attendeesCount_->setVisible(!degradedMode_);
  refreshAttendeeDetails();
}

void MainWindow::searchEvent() {
  const QString query = eventSearch_->text().trimmed();
  if (query.isEmpty())
    return;
  if (const Event *e = manager_.findEventByName(query.toStdString())) {
    if (selectById(eventsTable_, e->id())) {
      toast_->showMessage(
          QString("Showing %1").arg(Theme::quoted(toQ(e->name()))));
      return;
    }
  }
  for (int row = 0; row < eventsTable_->rowCount(); ++row) {
    if (!eventsTable_->isRowHidden(row)) {
      eventsTable_->selectRow(row);
      toast_->showMessage(
          QString("Showing %1")
              .arg(Theme::quoted(eventsTable_->item(row, EventCol::Name)->text())));
      return;
    }
  }
}

void MainWindow::searchAttendee() {
  const QString query = attendeeSearch_->text().trimmed();
  if (query.isEmpty())
    return;
  if (const Attendee *a = manager_.findAttendeeByEmail(query.toStdString())) {
    if (selectById(attendeesTable_, a->id)) {
      toast_->showMessage(QString("Showing %1").arg(toQ(a->name)));
      return;
    }
  }
  for (int row = 0; row < attendeesTable_->rowCount(); ++row) {
    if (!attendeesTable_->isRowHidden(row)) {
      attendeesTable_->selectRow(row);
      toast_->showMessage(QString("Showing %1")
                              .arg(attendeesTable_->item(row, AttendeeCol::Name)->text()));
      return;
    }
  }
}

bool MainWindow::save() {
  if (degradedMode_) {
    toast_->showMessage(
        "Resolve the data file issue before saving to this file");
    return false;
  }
  try {
    QDir().mkpath(QFileInfo(dataPath_).absolutePath());
    Storage::save(manager_, dataPath_.toStdString());
    dirty_ = false;
    updateTitle();
    updateSidebarState();
    return true;
  } catch (const std::exception &ex) {
    showError("Couldn't Save",
              QString("%1\n\nCheck that %2 is writable, then save again.")
                  .arg(friendlyError(ex), dataPath_));
    return false;
  }
}

void MainWindow::load() {
  if (dirty_ &&
      !ConfirmDialog::ask(this, "Discard Changes?",
                          "Reload from file and discard unsaved changes?",
                          "Discard and Reload", "Keep Changes", true)) {
    return;
  }
  try {
    Storage::load(manager_, dataPath_.toStdString());
    undoStack_.clear();
    dirty_ = false;
    clearDegradedMode();
    refreshAll();
    updateTitle();
    toast_->showMessage("Reloaded from " + QFileInfo(dataPath_).fileName());
  } catch (const std::exception &ex) {
    enterDegradedMode(friendlyError(ex));
    refreshAll();
    showError("Couldn't Reload",
              QString("%1\n\nCheck that %2 exists and is a valid data file.")
                  .arg(friendlyError(ex), dataPath_));
  }
}

void MainWindow::restoreFromBackup() {
  if (!Storage::hasBackup(dataPath_.toStdString())) {
    const QString msg = QString("No backup file (%1.bak) exists to restore.")
                            .arg(QFileInfo(dataPath_).fileName());
    if (toast_)
      toast_->showMessage(msg);
    else
      ConfirmDialog::inform(this, "No Backup Found", msg);
    return;
  }
  try {
    Storage::restoreBackup(manager_, dataPath_.toStdString());
    undoStack_.clear();
    dirty_ = false;
    clearDegradedMode();
    refreshAll();
    updateTitle();
    toast_->showMessage("Restored data from backup");
  } catch (const std::exception &ex) {
    showError("Couldn't Restore Backup", friendlyError(ex));
  }
}

void MainWindow::chooseDataFile() {
  const QString path = QFileDialog::getOpenFileName(
      this, "Open Data File", QFileInfo(dataPath_).absolutePath(),
      "Database Files (*.db);;All Files (*)");
  if (path.isEmpty())
    return;
  dataPath_ = path;
  load();
}

void MainWindow::startFreshFromCorrupt() {
  if (!ConfirmDialog::ask(
          this, "Start With Empty List?",
          "The unreadable file will be renamed with .corrupt so you keep a "
          "copy, and a fresh empty database will start.",
          "Start Fresh", "Keep File", true)) {
    return;
  }
  const QString archived = dataPath_ + ".corrupt";
  QFile::remove(archived);
  QFile::rename(dataPath_, archived);
  manager_.clear();
  undoStack_.clear();
  dirty_ = false;
  clearDegradedMode();
  refreshAll();
  updateTitle();
  toast_->showMessage("Started fresh database");
}

void MainWindow::exportCsv() {
  const QString path = QFileDialog::getSaveFileName(
      this, "Export CSV", QFileInfo(dataPath_).absolutePath() + "/events.csv",
      "CSV Files (*.csv)");
  if (path.isEmpty())
    return;
  try {
    Storage::exportCsv(manager_, path.toStdString());
    toast_->showMessage("Exported CSV to " + QFileInfo(path).fileName());
  } catch (const std::exception &ex) {
    showError("Couldn't Export CSV", friendlyError(ex));
  }
}

void MainWindow::closeEvent(QCloseEvent *event) {
  if (!dirty_) {
    event->accept();
    return;
  }
  ConfirmDialog box(
      "Unsaved Changes",
      "Your latest events, attendees and bookings haven't been saved.",
      "Save Changes", "Keep Editing", false, "Discard Changes", this);
  box.exec();
  if (box.result() == ConfirmDialog::Result::Cancel ||
      (box.result() == ConfirmDialog::Result::Primary && !save())) {
    event->ignore();
  } else {
    event->accept();
  }
}

void MainWindow::setDarkMode(bool dark) {
  Theme::apply(*qobject_cast<QApplication *>(QApplication::instance()),
               dark ? Theme::Mode::Dark : Theme::Mode::Light,
               Theme::contrast());
  QSettings().setValue("appearance", dark ? "dark" : "light");
  refreshAll();
}

void MainWindow::setHighContrast(bool high) {
  Theme::apply(*qobject_cast<QApplication *>(QApplication::instance()),
               Theme::mode(),
               high ? Theme::Contrast::High : Theme::Contrast::Standard);
  QSettings().setValue("highContrast", high);
  refreshAll();
}

void MainWindow::setAutoSave(bool enabled) {
  autoSave_ = enabled;
  QSettings().setValue("autoSave", enabled);
  if (autoSave_ && dirty_ && !degradedMode_)
    save();
  updateSidebarState();
  commands_.syncActions();
}

void MainWindow::undo() {
  if (!undoStack_.undo(manager_))
    return;
  markDirty();
  refreshAll();
  toast_->showMessage("Undid last action");
}

void MainWindow::markDirty(const QString &undoToastText) {
  dirty_ = true;
  updateTitle();
  if (autoSave_ && !degradedMode_)
    save();
  updateSidebarState();
  commands_.syncActions();

  if (!undoToastText.isEmpty()) {
    if (undoStack_.canUndo()) {
      toast_->showMessage(undoToastText, "Undo", [this] { undo(); }, "Ctrl+Z");
    } else {
      toast_->showMessage(undoToastText);
    }
  }
}

void MainWindow::updateTitle() {
  setWindowTitle(QString("Smart Event Management%1").arg(dirty_ ? " *" : ""));
}

void MainWindow::showError(const QString &title, const QString &message) {
  ConfirmDialog::inform(this, title, message);
}
