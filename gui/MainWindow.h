#pragma once
#include "CommandRegistry.h"
#include "EventManager.h"
#include "Snapshot.h"
#include <QHash>
#include <QMainWindow>
#include <QString>

class ActionToast;
class CommandPalette;
class HoverDelegate;
class QAbstractItemView;
class QCloseEvent;
class QComboBox;
class QEvent;
class QFrame;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QMenu;
class QObject;
class QProgressBar;
class QPushButton;
class QResizeEvent;
class QShowEvent;
class QSplitter;
class QStackedWidget;
class QTableWidget;
class QVBoxLayout;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  enum class ViewMode { Events = 0, Attendees = 1 };
  enum class StatusFilter { All = 0, Open = 1, AlmostFull = 2, Full = 3 };

  explicit MainWindow(const QString &dataPath, QWidget *parent = nullptr);

  // Accessors used by GUI verification tests.
  bool isDegraded() const { return degradedMode_; }
  ViewMode activeView() const { return activeView_; }
  CommandRegistry &commandRegistry() { return commands_; }
  CommandPalette *commandPalette() const { return palette_; }
  ActionToast *actionToast() const { return toast_; }
  int eventRowCount() const;
  int eventsPageIndex() const;
  int attendeesPageIndex() const;
  QLineEdit *eventSearchInput() const { return eventSearch_; }
  QComboBox *quickBookCombo() const { return quickBookCombo_; }

protected:
  void closeEvent(QCloseEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;
  void showEvent(QShowEvent *event) override;
  bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
  void addEvent();
  void editEvent();
  void deleteEvent();
  void registerAttendee();
  void editAttendee();
  void deleteAttendee();
  void bookFromEventInspector();
  void bookFromAttendeeInspector();
  void cancelSelectedBooking();
  void cancelAttendeeQuickBooking();
  void searchEvent();
  void searchAttendee();
  void filterEvents(const QString &query);
  void filterAttendees(const QString &query);
  bool save();
  void load();
  void restoreFromBackup();
  void chooseDataFile();
  void startFreshFromCorrupt();
  void exportCsv();
  void undo();
  void refreshDetails();
  void refreshAttendeeDetails();
  void updateActions();
  void setDarkMode(bool dark);
  void setHighContrast(bool high);
  void setAutoSave(bool enabled);

private:
  void buildUi();
  void registerCommands();
  void buildMenus();
  QWidget *buildSidebarRail();
  QWidget *buildCompactBar();
  QWidget *buildDegradedBanner();
  QWidget *buildEventsView();
  QWidget *buildAttendeesView();
  QWidget *buildEventInspector();
  QWidget *buildAttendeeInspector();
  void trackHover(QAbstractItemView *view, HoverDelegate *delegate);
  void switchView(ViewMode mode);
  void setStatusFilter(StatusFilter filter);
  void focusActiveSearch();
  void refreshAll();
  void refreshQuickBookCombos(bool preferUnbooked = false);
  void markDirty(const QString &undoToastText = QString());
  void updateTitle();
  void updateSidebarState();
  void updateDegradedBanner();
  void updateResponsiveLayout();
  void updateTableColumns();
  void enterDegradedMode(const QString &reason);
  void clearDegradedMode();
  void showError(const QString &title, const QString &message);
  void cancelBookingByIds(int eventId, int attendeeId);
  void jumpToEvent(int eventId);
  void jumpToAttendee(int attendeeId);
  QString attendeeName(int attendeeId) const;
  int selectedEventId() const;
  int selectedAttendeeId() const;
  int quickBookAttendeeId() const;
  int quickBookEventId() const;
  StatusFilter eventStatus(const Event *e) const;

  EventManager manager_;
  SnapshotStack undoStack_;
  CommandRegistry commands_;
  QString dataPath_;
  bool dirty_ = false;
  bool autoSave_ = true;
  bool degradedMode_ = false;
  QString degradedReason_;
  ViewMode activeView_ = ViewMode::Events;
  StatusFilter statusFilter_ = StatusFilter::All;
  QHash<QObject *, HoverDelegate *> hoverViewports_;

  CommandPalette *palette_ = nullptr;
  ActionToast *toast_ = nullptr;

  // Left rail & compact top bar
  QWidget *sidebarRail_ = nullptr;
  QWidget *compactBar_ = nullptr;
  QLabel *compactStatusDot_ = nullptr;
  QPushButton *compactEventsBtn_ = nullptr;
  QPushButton *compactAttendeesBtn_ = nullptr;
  QPushButton *compactMenuBtn_ = nullptr;
  QMenu *appMenu_ = nullptr;
  QPushButton *appMenuBtn_ = nullptr;
  QLabel *railStatusDot_ = nullptr;
  QLabel *railStatusText_ = nullptr;
  QPushButton *navEventsBtn_ = nullptr;
  QLabel *navEventsBadge_ = nullptr;
  QPushButton *navAttendeesBtn_ = nullptr;
  QLabel *navAttendeesBadge_ = nullptr;
  QPushButton *segFilterButtons_[4] = {};
  QPushButton *railAutoSaveBtn_ = nullptr;
  QLabel *railAutoSaveBadge_ = nullptr;
  QPushButton *railDarkBtn_ = nullptr;
  QLabel *railDarkBadge_ = nullptr;
  QPushButton *railContrastBtn_ = nullptr;
  QLabel *railContrastBadge_ = nullptr;

  // Degraded banner
  QFrame *degradedBanner_ = nullptr;
  QLabel *degradedLabel_ = nullptr;
  QPushButton *restoreBackupBtn_ = nullptr;

  // Central workspace stack
  QSplitter *workspaceSplitter_ = nullptr;
  QStackedWidget *viewStack_ = nullptr;
  QStackedWidget *inspectorOuterStack_ = nullptr;

  // Events view
  QFrame *eventsSegFrame_ = nullptr;
  QPushButton *eventsNewBtn_ = nullptr;
  QTableWidget *eventsTable_ = nullptr;
  QLineEdit *eventSearch_ = nullptr;
  QLabel *eventsCount_ = nullptr;
  QLabel *eventsFilterEmptyLabel_ = nullptr;
  QStackedWidget *eventsStack_ = nullptr;

  // Attendees view
  QPushButton *attendeesRegisterBtn_ = nullptr;
  QTableWidget *attendeesTable_ = nullptr;
  QLineEdit *attendeeSearch_ = nullptr;
  QLabel *attendeesCount_ = nullptr;
  QLabel *attendeesFilterEmptyLabel_ = nullptr;
  QStackedWidget *attendeesStack_ = nullptr;

  // Event Inspector
  QFrame *eventInspectorPanel_ = nullptr;
  QStackedWidget *eventInspectorStack_ = nullptr;
  QLabel *eventTitle_ = nullptr;
  QLabel *eventMeta_ = nullptr;
  QLabel *eventDescription_ = nullptr;
  QLabel *seatsLabel_ = nullptr;
  QLabel *statusChip_ = nullptr;
  QProgressBar *seatMeter_ = nullptr;
  QPushButton *editEventBtn_ = nullptr;
  QPushButton *deleteEventBtn_ = nullptr;
  QComboBox *quickBookCombo_ = nullptr;
  QLabel *actionHint_ = nullptr;
  QLabel *quickBookStateChip_ = nullptr;
  QPushButton *bookButton_ = nullptr;
  QPushButton *cancelButton_ = nullptr;
  QLabel *confirmedCount_ = nullptr;
  QListWidget *confirmedList_ = nullptr;
  QStackedWidget *confirmedStack_ = nullptr;
  QLabel *waitlistCount_ = nullptr;
  QListWidget *waitlistList_ = nullptr;
  QStackedWidget *waitlistStack_ = nullptr;
  QVBoxLayout *eventDetailLayout_ = nullptr;
  QWidget *detailTailSpacer_ = nullptr;

  // Attendee Inspector
  QFrame *attendeeInspectorPanel_ = nullptr;
  QStackedWidget *attendeeInspectorStack_ = nullptr;
  QLabel *attendeeTitle_ = nullptr;
  QLabel *attendeeEmail_ = nullptr;
  QLabel *attendeeSummary_ = nullptr;
  QPushButton *editAttendeeBtn_ = nullptr;
  QPushButton *deleteAttendeeBtn_ = nullptr;
  QComboBox *quickEventCombo_ = nullptr;
  QLabel *attendeeActionHint_ = nullptr;
  QLabel *attendeeBookStateChip_ = nullptr;
  QPushButton *bookEventBtn_ = nullptr;
  QPushButton *cancelEventBookingBtn_ = nullptr;
  QLabel *attendeeBookingsCount_ = nullptr;
  QListWidget *attendeeBookingsList_ = nullptr;
  QStackedWidget *attendeeBookingsStack_ = nullptr;
  QVBoxLayout *attendeeDetailLayout_ = nullptr;
  QWidget *attendeeTailSpacer_ = nullptr;
};
