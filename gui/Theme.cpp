#include "Theme.h"
#include <QApplication>
#include <QHash>
#include <QPalette>
#include <QStyle>
#include <QStyleFactory>
#include <QWidget>

namespace {

#include "ThemeTokens.inc"

Theme::Mode g_mode = Theme::Mode::Light;
QHash<QString, QColor> g_colors;

void build(const char* const* n, const char* const* a, const char* const* s, const char* const* w,
           const char* const* d, const char* control) {
    g_colors.clear();
    auto set = [](const char* role, const char* hex) { g_colors.insert(QString::fromLatin1(role), QColor(QString::fromLatin1(hex))); };

    // Neutral roles (step numbers are 1-based in docs/DESIGN.md; arrays are 0-based).
    set("panel", n[0]);          // step 1: raised surface
    set("app", n[1]);            // step 2: window background
    set("fill", n[2]);           // step 3: hover fill
    set("fill-hover", n[3]);     // step 4
    set("fill-active", n[4]);    // step 5: pressed, meter track
    set("border-subtle", n[5]);  // step 6: panel outline
    set("border", n[6]);         // step 7: component border
    set("border-strong", n[7]);  // step 8: also disabled text
    set("control", control);     // input outline, >= 3:1 on panel
    set("text-off", n[7]);
    set("text-2", n[10]);        // step 11
    set("text", n[11]);          // step 12
    set("on-solid", "#ffffff");

    set("accent-bg", a[2]);
    set("accent-bg-hover", a[3]);
    set("accent-bg-active", a[4]);
    set("accent-border", a[6]);
    set("accent", a[8]);
    set("accent-hover", a[9]);
    set("accent-text", a[10]);

    set("success-bg", s[2]);
    set("success", s[8]);
    set("success-text", s[10]);
    set("warning-bg", w[2]);
    set("warning", w[8]);
    set("warning-text", w[10]);
    set("danger-bg", d[2]);
    set("danger-border", d[6]);
    set("danger", d[8]);
    set("danger-text", d[10]);
}

// Tokens are written as @{role}. Everything visual lives here; widgets only set properties.
const char* const kQss = R"QSS(
QWidget { color: @{text}; }
QMainWindow, QWidget#central { background: @{app}; }
QDialog, QMessageBox { background: @{panel}; }
QToolTip { background: @{panel}; color: @{text}; border: 1px solid @{border}; padding: 4px 8px; }

QMenuBar { background: @{app}; padding: 4px 8px 0 8px; }
QMenuBar::item { background: transparent; padding: 4px 8px; border-radius: 6px; }
QMenuBar::item:selected { background: @{fill-hover}; }
QMenu { background: @{panel}; border: 1px solid @{border-subtle}; padding: 4px; }
QMenu::item { padding: 6px 28px 6px 12px; border-radius: 4px; }
QMenu::item:selected { background: @{fill-hover}; }
QMenu::item:disabled { color: @{text-off}; }
QMenu::separator { height: 1px; background: @{border-subtle}; margin: 4px 8px; }

QFrame#panel { background: @{panel}; border: 1px solid @{border-subtle}; border-radius: 12px; }
QFrame#well { background: @{app}; border: none; border-radius: 6px; }
QSplitter::handle { background: transparent; }

QLabel { background: transparent; }
QLabel[role="panelTitle"] { font-weight: 600; }
QLabel[role="fieldLabel"] { font-size: 12px; font-weight: 600; color: @{text-2}; }
QLabel[role="caption"] { font-size: 12px; color: @{text-2}; }
QLabel[role="muted"] { color: @{text-2}; }
QLabel[role="error"] { font-size: 12px; color: @{danger-text}; }
QLabel[role="emptyTitle"] { font-weight: 600; }
QLabel[chip="success"] { background: @{success-bg}; color: @{success-text}; font-size: 12px; font-weight: 600; padding: 2px 8px; border-radius: 10px; }
QLabel[chip="warning"] { background: @{warning-bg}; color: @{warning-text}; font-size: 12px; font-weight: 600; padding: 2px 8px; border-radius: 10px; }
QLabel[chip="danger"] { background: @{danger-bg}; color: @{danger-text}; font-size: 12px; font-weight: 600; padding: 2px 8px; border-radius: 10px; }

QPushButton { background: @{panel}; color: @{text}; border: 1px solid @{border}; border-radius: 6px;
              padding: 5px 12px; min-height: 20px; font-weight: 600; }
QPushButton:hover { background: @{fill}; }
QPushButton:pressed { background: @{fill-hover}; }
QPushButton:focus { border: 2px solid @{accent}; padding: 4px 11px; }
QPushButton:disabled { background: @{fill}; color: @{text-off}; border-color: transparent; }
QPushButton[variant="primary"] { background: @{accent}; color: @{on-solid}; border-color: @{accent}; }
QPushButton[variant="primary"]:hover { background: @{accent-hover}; border-color: @{accent-hover}; }
QPushButton[variant="primary"]:pressed { background: @{accent-hover}; }
QPushButton[variant="primary"]:focus { border: 2px solid @{text}; padding: 4px 11px; }
QPushButton[variant="primary"]:disabled { background: @{fill}; color: @{text-off}; border-color: transparent; }
QPushButton[variant="danger"] { color: @{danger-text}; border-color: @{danger-border}; }
QPushButton[variant="danger"]:hover { background: @{danger-bg}; }
QPushButton[variant="danger"]:pressed { background: @{danger-bg}; }
QPushButton[variant="danger"]:focus { border: 2px solid @{danger}; padding: 4px 11px; }
QPushButton[variant="danger"]:disabled { background: @{fill}; color: @{text-off}; border-color: transparent; }
QPushButton[variant="ghost"] { background: transparent; border-color: transparent; }
QPushButton[variant="ghost"]:hover { background: @{fill}; }
QPushButton[variant="ghost"]:disabled { background: transparent; color: @{text-off}; }

QLineEdit, QSpinBox, QDateEdit { background: @{panel}; color: @{text}; border: 1px solid @{control}; border-radius: 6px;
              padding: 5px 9px; min-height: 20px;
              selection-background-color: @{accent-bg-active}; selection-color: @{text}; }
QLineEdit:hover, QSpinBox:hover, QDateEdit:hover { border-color: @{text-2}; }
QLineEdit:focus, QSpinBox:focus, QDateEdit:focus { border: 2px solid @{accent}; padding: 4px 8px; }
QLineEdit:disabled, QSpinBox:disabled, QDateEdit:disabled { background: @{fill}; color: @{text-off}; border-color: @{border-subtle}; }
QLineEdit[invalid="true"], QSpinBox[invalid="true"], QDateEdit[invalid="true"] { border: 1px solid @{danger}; }
QLineEdit[invalid="true"]:focus { border: 2px solid @{danger}; }

QAbstractScrollArea { background: transparent; border: none; }
QTableWidget, QListWidget { background: transparent; border: none; gridline-color: transparent; outline: 0; }
QTableWidget::item, QListWidget::item { border: none; }
QHeaderView { background: transparent; }
QHeaderView::section { background: transparent; color: @{text-2}; border: none; border-bottom: 1px solid @{border-subtle};
                       padding: 6px 12px; font-size: 12px; font-weight: 600; }
QTableCornerButton::section { background: transparent; border: none; }

QScrollBar:vertical { background: transparent; width: 10px; margin: 0; }
QScrollBar:horizontal { background: transparent; height: 10px; margin: 0; }
QScrollBar::handle:vertical { background: @{border}; border-radius: 3px; min-height: 28px; margin: 2px; }
QScrollBar::handle:horizontal { background: @{border}; border-radius: 3px; min-width: 28px; margin: 2px; }
QScrollBar::handle:vertical:hover, QScrollBar::handle:horizontal:hover { background: @{border-strong}; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }

QProgressBar { background: @{fill-active}; border: none; border-radius: 3px; min-height: 6px; max-height: 6px; }
QProgressBar::chunk { background: @{accent}; border-radius: 3px; }
QProgressBar[state="warn"]::chunk { background: @{warning}; }
QProgressBar[state="full"]::chunk { background: @{danger}; }

QStatusBar { background: @{app}; color: @{text-2}; }
QStatusBar::item { border: none; }

QMessageBox QLabel { min-width: 320px; }
QCalendarWidget QAbstractItemView { background: @{panel}; color: @{text}; selection-background-color: @{accent};
                                    selection-color: @{on-solid}; outline: 0; }
QCalendarWidget QWidget#qt_calendar_navigationbar { background: @{panel}; }
)QSS";

QString expand(QString qss) {
    for (auto it = g_colors.constBegin(); it != g_colors.constEnd(); ++it)
        qss.replace(QStringLiteral("@{") + it.key() + QLatin1Char('}'), it.value().name());
    return qss;
}

}  // namespace

namespace Theme {

Mode systemMode() {
    const QPalette p = QApplication::palette();
    return p.color(QPalette::Window).lightness() < p.color(QPalette::WindowText).lightness() ? Mode::Dark : Mode::Light;
}

Mode mode() { return g_mode; }

void apply(QApplication& app, Mode m) {
    g_mode = m;
    if (m == Mode::Dark) build(kDarkNeutral, kDarkAccent, kDarkSuccess, kDarkWarning, kDarkDanger, kDarkControl);
    else build(kLightNeutral, kLightAccent, kLightSuccess, kLightWarning, kLightDanger, kLightControl);

    static bool styled = false;
    if (!styled) {
        // Fusion gives every platform the same, themeable base to paint our roles onto.
        if (QStyle* fusion = QStyleFactory::create(QStringLiteral("Fusion"))) app.setStyle(fusion);
        styled = true;
    }

    QPalette pal;
    pal.setColor(QPalette::Window, color("app"));
    pal.setColor(QPalette::WindowText, color("text"));
    pal.setColor(QPalette::Base, color("panel"));
    pal.setColor(QPalette::AlternateBase, color("fill"));
    pal.setColor(QPalette::Text, color("text"));
    pal.setColor(QPalette::Button, color("panel"));
    pal.setColor(QPalette::ButtonText, color("text"));
    pal.setColor(QPalette::Highlight, color("accent"));
    pal.setColor(QPalette::HighlightedText, color("on-solid"));
    pal.setColor(QPalette::ToolTipBase, color("panel"));
    pal.setColor(QPalette::ToolTipText, color("text"));
    pal.setColor(QPalette::Link, color("accent-text"));
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
    pal.setColor(QPalette::PlaceholderText, color("text-2"));
#endif
    pal.setColor(QPalette::Disabled, QPalette::WindowText, color("text-off"));
    pal.setColor(QPalette::Disabled, QPalette::Text, color("text-off"));
    pal.setColor(QPalette::Disabled, QPalette::ButtonText, color("text-off"));
    app.setPalette(pal);

    QFont font = app.font();
    font.setPixelSize(13);
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
    font.setFamilies(QStringList{QStringLiteral("Inter"), QStringLiteral("Segoe UI"), QStringLiteral("SF Pro Text"),
                                 QStringLiteral("Helvetica Neue"), QStringLiteral("Noto Sans")});
#endif
    app.setFont(font);

    app.setStyleSheet(expand(QString::fromLatin1(kQss)));
}

QColor color(const char* role) { return g_colors.value(QString::fromLatin1(role)); }

QFont titleFont(int pixelSize) {
    QFont f;
    f.setPixelSize(pixelSize);
    f.setWeight(QFont::DemiBold);
    f.setLetterSpacing(QFont::AbsoluteSpacing, pixelSize >= 40 ? -0.06 * pixelSize : pixelSize >= 24 ? -0.04 * pixelSize : -0.02 * pixelSize);
    return f;
}

void repolish(QWidget* widget) {
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}

QString quoted(const QString& text) {
    return QString::fromUtf8("\xE2\x80\x9C") + text + QString::fromUtf8("\xE2\x80\x9D");
}

}  // namespace Theme
