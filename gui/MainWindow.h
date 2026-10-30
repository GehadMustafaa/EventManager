#pragma once
#include <QMainWindow>
#include <QString>
#include "EventManager.h"

class QCloseEvent;
class QLabel;
class QLineEdit;
class QListWidget;
class QTableWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(const QString& dataPath, QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

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

private:
    void buildUi();
    void refreshAll();
    void markDirty();
    void updateTitle();
    void showError(const QString& message);
    QString attendeeLabel(int attendeeId) const;

    EventManager manager_;
    QString dataPath_;
    bool dirty_ = false;

    QTableWidget* eventsTable_ = nullptr;
    QTableWidget* attendeesTable_ = nullptr;
    QLineEdit* eventSearch_ = nullptr;
    QLineEdit* attendeeSearch_ = nullptr;
    QLabel* detailsTitle_ = nullptr;
    QListWidget* confirmedList_ = nullptr;
    QListWidget* waitlistList_ = nullptr;
};
