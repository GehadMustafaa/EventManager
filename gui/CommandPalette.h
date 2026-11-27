#pragma once
#include <QDialog>
#include <QPointer>
#include <QString>
#include <QVector>
#include <functional>

class CommandRegistry;
class QLineEdit;
class QListWidget;
class QWidget;

struct PaletteJumpItem {
    QString category;
    QString title;
    QString subtitle;
    std::function<void()> trigger;
};

class CommandPalette : public QDialog {
    Q_OBJECT

public:
    using JumpProvider = std::function<QVector<PaletteJumpItem>(const QString& query)>;

    explicit CommandPalette(CommandRegistry& registry, QWidget* parent = nullptr);

    void setJumpProvider(JumpProvider provider);
    void openPalette();

signals:
    void commandExecuted(const QString& title, const QString& shortcutText);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    void populate(const QString& query);
    void activateCurrent();
    void moveSelection(int delta);
    void restoreFocus();

    CommandRegistry& registry_;
    JumpProvider jumpProvider_;
    QLineEdit* input_ = nullptr;
    QListWidget* list_ = nullptr;
    QPointer<QWidget> previousFocus_;
    QVector<std::function<void()>> rowCallbacks_;
};
