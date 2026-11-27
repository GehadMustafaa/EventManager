#pragma once
#include <QDialog>
#include <QString>

class QPushButton;

class ConfirmDialog : public QDialog {
public:
    enum class Result { Primary, Secondary, Cancel };

    ConfirmDialog(const QString& title, const QString& body, const QString& confirmLabel,
                  const QString& cancelLabel = QStringLiteral("Cancel"), bool destructive = false,
                  const QString& secondaryLabel = QString(), QWidget* parent = nullptr);

    Result result() const { return result_; }

    static bool ask(QWidget* parent, const QString& title, const QString& body,
                    const QString& confirmLabel, const QString& cancelLabel = QStringLiteral("Cancel"),
                    bool destructive = false);
    static void inform(QWidget* parent, const QString& title, const QString& body);

protected:
    void showEvent(QShowEvent* event) override;

private:
    Result result_ = Result::Cancel;
    QPushButton* safeDefaultBtn_ = nullptr;
};
