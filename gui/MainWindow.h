#pragma once
#include <QHash>
#include <QMainWindow>
#include <QString>
#include "EventManager.h"

class HoverDelegate;
class QAbstractItemView;
class QAction;
class QCloseEvent;
class QEvent;
class QLabel;
class QLineEdit;
class QListWidget;
class QObject;
class QProgressBar;
class QPushButton;
class QStackedWidget;
class QTableWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(const QString& dataPath, QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void addEvent();
    void deleteEvent();
    void registerAttendee();
    void bookSelected();
    void cancelSelected();
    void searchEvent();
    void searchAttendee();
    bool save();
    void load();
    void refreshDetails();
    void updateActions();
    void setDarkMode(bool dark);

private:
    void buildUi();
    void buildMenus();
    QWidget* buildEventsPanel();
    QWidget* buildAttendeesPanel();
    QWidget* buildInspector();
    void trackHover(QAbstractItemView* view, HoverDelegate* delegate);
    void refreshAll();
    void markDirty();
    void updateTitle();
    void showError(const QString& title, const QString& message);
    QString attendeeName(int attendeeId) const;
    int selectedAttendeeId() const;

    EventManager manager_;
    QString dataPath_;
    bool dirty_ = false;
    QHash<QObject*, HoverDelegate*> hoverViewports_;

    QAction* darkModeAction_ = nullptr;

    // Events panel
    QTableWidget* eventsTable_ = nullptr;
    QLineEdit* eventSearch_ = nullptr;
    QLabel* eventsCount_ = nullptr;
    QStackedWidget* eventsStack_ = nullptr;
    QPushButton* deleteEventButton_ = nullptr;

    // Attendees panel
    QTableWidget* attendeesTable_ = nullptr;
    QLineEdit* attendeeSearch_ = nullptr;
    QLabel* attendeesCount_ = nullptr;
    QStackedWidget* attendeesStack_ = nullptr;

    // Inspector
    QStackedWidget* inspectorStack_ = nullptr;
    QLabel* eventTitle_ = nullptr;
    QLabel* eventMeta_ = nullptr;
    QLabel* seatsLabel_ = nullptr;
    QLabel* statusChip_ = nullptr;
    QProgressBar* seatMeter_ = nullptr;
    QLabel* actionName_ = nullptr;
    QLabel* actionHint_ = nullptr;
    QPushButton* bookButton_ = nullptr;
    QPushButton* cancelButton_ = nullptr;
    QLabel* confirmedCount_ = nullptr;
    QListWidget* confirmedList_ = nullptr;
    QStackedWidget* confirmedStack_ = nullptr;
    QLabel* waitlistCount_ = nullptr;
    QListWidget* waitlistList_ = nullptr;
    QStackedWidget* waitlistStack_ = nullptr;

    QLabel* dirtyChip_ = nullptr;
};
