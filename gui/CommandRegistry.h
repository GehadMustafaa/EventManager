#pragma once
#include <QKeySequence>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <functional>

class QAction;
class QMenu;
class QWidget;

struct CommandSpec {
    QString id;
    QString title;
    QStringList aliases;
    QString category;
    QKeySequence shortcut;
    bool checkable = false;
    std::function<bool()> isEnabled;
    std::function<bool()> isChecked;
    std::function<int()> contextBoost;
    std::function<void()> trigger;
};

struct CommandMatch {
    const CommandSpec* spec = nullptr;
    int score = 0;
    QString matchedAlias;
    bool enabled = true;
};

class CommandRegistry : public QObject {
    Q_OBJECT

public:
    explicit CommandRegistry(QObject* parent = nullptr);

    void registerCommand(CommandSpec spec);
    QAction* bindAction(const QString& id, QWidget* shortcutOwner, QMenu* menu = nullptr);
    QAction* actionFor(const QString& id) const;
    void syncActions();

    const CommandSpec* find(const QString& id) const;
    QVector<CommandMatch> search(const QString& query) const;
    bool trigger(const QString& id) const;

    static int fuzzyScore(const QString& query, const QString& candidate);

private:
    struct Entry {
        CommandSpec spec;
        QAction* action = nullptr;
    };

    int indexOf(const QString& id) const;

    QVector<Entry> entries_;
};
