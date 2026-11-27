#include "Theme.h"
#include <QAbstractSpinBox>
#include <QApplication>
#include <QComboBox>
#include <QFocusEvent>
#include <QHash>
#include <QLineEdit>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QProxyStyle>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleOption>
#include <QWidget>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {

#include "ThemeTokens.inc"

Theme::Mode g_mode = Theme::Mode::Light;
bool g_highContrast = false;
QHash<QString, QColor> g_colors;

void build(const char *const *n, const char *const *a, const char *const *s,
           const char *const *w, const char *const *d, const char *control,
           const char *textOff, const char *onSolid, bool dark,
           bool highContrast) {
  g_colors.clear();
  auto set = [](const char *role, const char *hex) {
    g_colors.insert(QString::fromLatin1(role),
                    QColor(QString::fromLatin1(hex)));
  };

  // Neutral roles: in Light mode n[0] is brightest raised surface; in Dark mode
  // n[0] is deepest app canvas.
  set("panel", dark ? n[1] : n[0]); // raised surface (cards, inputs)
  set("app",
      dark ? n[0] : n[1]); // window background, sidebar rail, sunken well
  set("seg-active",
      dark ? n[3] : n[0]); // selected segmented tab lifts above tray
  set("fill", n[2]);       // hover fill
  set("fill-hover", n[3]);
  set("fill-active", n[4]);                         // pressed, meter track
  set("border-subtle", highContrast ? n[6] : n[5]); // panel outline
  set("border", highContrast ? n[7] : n[6]);        // component border
  set("border-strong", n[7]);
  set("control", control);   // input outline, >= 3:1 on panel
  set("text-off", textOff);  // legible disabled foreground
  set("text-2", n[10]);      // secondary text in all modes
  set("text", n[11]);        // primary text
  set("on-solid", onSolid);

  set("accent-bg", a[2]);
  set("accent-bg-hover", a[3]);
  set("accent-bg-active", a[4]);
  set("accent-border", a[6]);
  set("accent", a[8]);
  set("accent-hover", a[9]);
  // Pressed solid button needs >= 4.5:1 against white text in both modes (step
  // 11 in dark is pastel text colour).
  g_colors.insert(QStringLiteral("accent-active"),
                  dark ? QColor(QString::fromLatin1(a[7]))
                       : QColor(QString::fromLatin1(a[9])).darker(115));
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

// Tokens are written as @{role}. Everything visual lives here; widgets only set
// properties.
const char *const kQss = R"QSS(
QWidget { color: @{text}; }
QMainWindow, QWidget#central { background: @{app}; }
QDialog, QMessageBox { background: @{panel}; }
QToolTip { background: @{panel}; color: @{text}; border: 1px solid @{border}; padding: 4px 8px; }

QMenuBar { background: @{app}; padding: 4px 8px 0 8px; }
QMenuBar::item { background: transparent; padding: 4px 8px; border-radius: 6px; }
QMenuBar::item:selected { background: @{fill-hover}; }
QMenu { background: @{panel}; border: 1px solid @{border-subtle}; border-radius: 12px; padding: 4px; }
QMenu::item { padding: 6px 28px 6px 12px; border-radius: 6px; }
QMenu::item:selected { background: @{fill-hover}; }
QMenu::item:disabled { color: @{text-off}; }
QMenu::separator { height: 1px; background: @{border-subtle}; margin: 4px 8px; }

QFrame#panel { background: @{panel}; border: 1px solid @{border-subtle}; border-radius: 12px; }
QFrame#well { background: @{app}; border: 1px solid @{border-subtle}; border-radius: 12px; }
QFrame#sidebarRail { background: @{app}; border-right: 1px solid @{border-subtle}; }
QFrame#compactBar { background: @{app}; border-bottom: 1px solid @{border-subtle}; }
QFrame#segmented { background: @{fill}; border: 1px solid @{border-subtle}; border-radius: 6px; padding: 2px; }
QFrame#commandPalette { background: @{panel}; border: 1px solid @{border}; border-radius: 12px; }
QFrame#toastPill { background: @{panel}; border: 1px solid @{border}; border-radius: 12px; }
QFrame#degradedBanner[severity="danger"] { background: @{danger-bg}; border: none; border-bottom: 1px solid @{danger-border}; border-radius: 0px; }
QFrame#degradedBanner[severity="warning"] { background: @{warning-bg}; border: none; border-bottom: 1px solid @{warning}; border-radius: 0px; }
QSplitter::handle { background: transparent; }

QLabel { background: transparent; }
QLabel[role="panelTitle"] { font-weight: 600; }
QLabel[role="fieldLabel"] { font-size: 12px; font-weight: 600; color: @{text-2}; }
QLabel[role="caption"] { font-size: 12px; color: @{text-2}; }
QLabel[role="muted"] { color: @{text-2}; }
QLabel[role="error"] { font-size: 12px; color: @{danger-text}; }
QLabel[role="emptyTitle"] { font-weight: 600; }
QLabel[role="navSection"] { font-size: 11px; font-weight: 600; color: @{text-2}; padding: 4px 10px; }
QLabel[role="kbd"] { background: @{fill}; color: @{text-2}; border: 1px solid @{border-subtle}; border-radius: 4px;
                     padding: 1px 6px; min-height: 16px; max-height: 20px; font-size: 11px; }
QLabel[chip="neutral"] { background: @{fill}; color: @{text-2}; font-size: 11px; font-weight: 600; padding: 1px 7px; border-radius: 9px; }
QLabel[chip="subtle"] { background: @{fill-active}; color: @{text}; font-size: 11px; font-weight: 600; padding: 1px 7px; border-radius: 9px; }
QLabel[chip="success"] { background: @{success-bg}; color: @{success-text}; font-size: 12px; font-weight: 600; padding: 2px 8px; border-radius: 10px; }
QLabel[chip="warning"] { background: @{warning-bg}; color: @{warning-text}; font-size: 12px; font-weight: 600; padding: 2px 8px; border-radius: 10px; }
QLabel[chip="danger"] { background: @{danger-bg}; color: @{danger-text}; font-size: 12px; font-weight: 600; padding: 2px 8px; border-radius: 10px; }
QLabel[statusDot="success"] { background: @{success}; border-radius: 4px; }
QLabel[statusDot="warning"] { background: @{warning}; border-radius: 4px; }
QLabel[statusDot="danger"] { background: @{danger}; border-radius: 4px; }

QPushButton { background: @{panel}; color: @{text}; border: 1px solid @{border}; border-radius: 6px;
              padding: 6px 12px; min-height: 20px; font-weight: 600; }
QPushButton:hover { background: @{fill}; }
QPushButton:pressed { background: @{fill-hover}; }
QPushButton[kbdFocus="true"] { border: 2px solid @{accent}; padding: 5px 11px; }
QPushButton:disabled { background: @{fill}; color: @{text-off}; border: 1px dashed @{border}; font-weight: 400; }
QPushButton[inactive="true"] { background: @{fill}; color: @{text-2}; border: 1px dashed @{border}; font-weight: 400; }
QPushButton[inactive="true"]:hover { background: @{fill-hover}; }
QPushButton[variant="primary"] { background: @{accent}; color: @{on-solid}; border-color: @{accent}; }
QPushButton[variant="primary"]:hover { background: @{accent-hover}; border-color: @{accent-hover}; }
QPushButton[variant="primary"]:pressed { background: @{accent-active}; border-color: @{accent-active}; }
QPushButton[variant="primary"][kbdFocus="true"] { border: 2px solid @{text}; padding: 5px 11px; }
QPushButton[variant="primary"]:disabled { background: @{fill}; color: @{text-off}; border: 1px dashed @{border}; font-weight: 400; }
QPushButton[variant="danger"] { color: @{danger-text}; border-color: @{danger-border}; }
QPushButton[variant="danger"]:hover { background: @{danger-bg}; }
QPushButton[variant="danger"]:pressed { background: @{danger-bg}; }
QPushButton[variant="danger"][kbdFocus="true"] { border: 2px solid @{danger}; padding: 5px 11px; }
QPushButton[variant="danger"]:disabled { background: @{fill}; color: @{text-off}; border: 1px dashed @{border}; font-weight: 400; }
QPushButton[variant="ghost"] { background: transparent; border-color: transparent; }
QPushButton[variant="ghost"]:hover { background: @{fill}; }
QPushButton[variant="ghost"]:pressed { background: @{fill-hover}; }
QPushButton[variant="ghost"][kbdFocus="true"] { border: 2px solid @{accent}; padding: 5px 11px; }
QPushButton[variant="ghost"]:disabled { background: transparent; color: @{text-off}; border: 1px dashed @{border-subtle}; font-weight: 400; }
QPushButton[variant="ghostDanger"] { background: transparent; border-color: transparent; color: @{text-2}; }
QPushButton[variant="ghostDanger"]:hover { background: @{danger-bg}; color: @{danger-text}; }
QPushButton[variant="ghostDanger"]:pressed { background: @{danger-bg}; color: @{danger-text}; }
QPushButton[variant="ghostDanger"][kbdFocus="true"] { border: 2px solid @{danger}; padding: 5px 11px; }
QPushButton[variant="ghostDanger"]:disabled { background: transparent; color: @{text-off}; border: 1px dashed @{border-subtle}; font-weight: 400; }
QPushButton[variant="icon"] { background: transparent; border: 1px solid transparent; border-radius: 6px;
                              padding: 0px; min-width: 24px; min-height: 24px; font-weight: 600; color: @{text-2}; }
QPushButton[variant="icon"]:hover { background: @{fill}; color: @{text}; }
QPushButton[variant="icon"]:pressed { background: @{fill-hover}; color: @{text}; }
QPushButton[variant="icon"][kbdFocus="true"] { border: 2px solid @{accent}; }
QPushButton[variant="icon"]:disabled { background: transparent; color: @{text-off}; border: 1px dashed @{border-subtle}; font-weight: 400; }
QPushButton[variant="nav"] { background: transparent; border: 1px solid transparent; border-radius: 6px;
                             padding: 4px 10px; min-height: 24px; text-align: left; font-weight: 400; color: @{text-2}; }
QPushButton[variant="nav"]:hover { background: @{fill}; color: @{text}; }
QPushButton[variant="nav"][active="true"] { background: @{fill-active}; color: @{text}; font-weight: 600; }
QPushButton[variant="nav"][kbdFocus="true"] { border: 2px solid @{accent}; padding: 3px 9px; }
QPushButton[variant="nav"] QLabel[role="navText"] { color: @{text-2}; font-weight: 400; }
QPushButton[variant="nav"]:hover QLabel[role="navText"] { color: @{text}; }
QPushButton[variant="nav"][active="true"] QLabel[role="navText"] { color: @{text}; font-weight: 600; }
QPushButton[variant="segment"] { background: transparent; border: 2px solid transparent; border-radius: 4px;
                                 padding: 3px 7px; min-height: 18px; font-size: 12px; font-weight: 400; color: @{text-2}; }
QPushButton[variant="segment"]:hover { color: @{text}; }
QPushButton[variant="segment"][kbdFocus="true"] { border-color: @{accent}; }
QPushButton[variant="segment"]:checked { background: @{seg-active}; color: @{text}; font-weight: 600; border: 1px solid @{border-subtle}; padding: 4px 8px; }
QPushButton[variant="segment"]:checked[kbdFocus="true"] { border: 2px solid @{accent}; padding: 3px 7px; }

QLineEdit, QSpinBox, QDateEdit, QTimeEdit, QComboBox, QPlainTextEdit {
              background: @{panel}; color: @{text}; border: 1px solid @{control}; border-radius: 6px;
              padding: 6px 9px; min-height: 20px;
              selection-background-color: @{accent-bg-active}; selection-color: @{text}; }
QComboBox, QDateEdit { padding-right: 26px; }
QComboBox QLineEdit, QComboBox QLineEdit:hover, QComboBox QLineEdit:focus, QComboBox QLineEdit:disabled {
              background: transparent; border: none; padding: 0; min-height: 0; }
QLineEdit:hover, QSpinBox:hover, QDateEdit:hover, QTimeEdit:hover, QComboBox:hover, QPlainTextEdit:hover { border-color: @{text-2}; }
QLineEdit[kbdFocus="true"], QSpinBox[kbdFocus="true"], QDateEdit[kbdFocus="true"], QTimeEdit[kbdFocus="true"], QComboBox[kbdFocus="true"], QPlainTextEdit[kbdFocus="true"] { border: 2px solid @{accent}; padding: 5px 8px; }
QComboBox[kbdFocus="true"], QDateEdit[kbdFocus="true"] { padding-right: 25px; }
QLineEdit:disabled, QSpinBox:disabled, QDateEdit:disabled, QTimeEdit:disabled, QComboBox:disabled, QPlainTextEdit:disabled {
              background: @{fill}; color: @{text-off}; border: 1px dashed @{border-subtle}; }
QLineEdit[invalid="true"], QSpinBox[invalid="true"], QDateEdit[invalid="true"], QTimeEdit[invalid="true"], QComboBox[invalid="true"], QPlainTextEdit[invalid="true"] {
              border: 1px solid @{danger}; }
QLineEdit[invalid="true"][kbdFocus="true"], QSpinBox[invalid="true"][kbdFocus="true"], QDateEdit[invalid="true"][kbdFocus="true"], QTimeEdit[invalid="true"][kbdFocus="true"], QComboBox[invalid="true"][kbdFocus="true"], QPlainTextEdit[invalid="true"][kbdFocus="true"] {
              border: 2px solid @{danger}; padding: 5px 8px; }
QLineEdit#paletteInput { background: transparent; border: none; border-bottom: 1px solid @{border-subtle};
                         border-radius: 0; padding: 12px 16px; font-size: 14px; }
QLineEdit#paletteInput[kbdFocus="true"] { border: none; border-bottom: 1px solid @{border-subtle}; padding: 12px 16px; }
QComboBox::drop-down, QDateEdit::drop-down { subcontrol-origin: border; subcontrol-position: center right; width: 24px; border: none; background: transparent; }
QSpinBox::up-button, QSpinBox::down-button { width: 16px; border: none; background: transparent; }
QComboBox QAbstractItemView { background: @{panel}; color: @{text}; border: 1px solid @{border-subtle}; border-radius: 6px; padding: 4px;
                            selection-background-color: @{accent-bg}; selection-color: @{text}; outline: 0; }

QTableView, QListView { background: transparent; border: none; }
QTableWidget, QListWidget { background: transparent; border: 1px solid transparent; border-radius: 6px; gridline-color: transparent; outline: 0; }
QTableWidget[kbdFocus="true"], QListWidget[kbdFocus="true"] { border: 2px solid @{accent}; }
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

class ChevronOverlay : public QWidget {
public:
  explicit ChevronOverlay(QWidget *target) : QWidget(target), target_(target) {
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    reposition();
    show();
  }

  void reposition() {
    if (!target_)
      return;
    constexpr int kWidth = 24;
    setGeometry(target_->width() - kWidth, 0, kWidth, target_->height());
    raise();
  }

protected:
  void paintEvent(QPaintEvent *) override {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    const bool enabled = target_ ? target_->isEnabled() : isEnabled();
    p.setPen(QPen(Theme::color(enabled ? "text-2" : "text-off"), 1.6,
                  Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);
    const QPointF c = QRectF(rect()).center() + QPointF(-1.0, 0.5);
    QPainterPath path;
    path.moveTo(c.x() - 4.0, c.y() - 2.0);
    path.lineTo(c.x(), c.y() + 2.2);
    path.lineTo(c.x() + 4.0, c.y() - 2.0);
    p.drawPath(path);
  }

private:
  QWidget *target_ = nullptr;
};

class ChevronResizeFilter : public QObject {
public:
  ChevronResizeFilter(QWidget *target, ChevronOverlay *overlay)
      : QObject(target), overlay_(overlay) {}

protected:
  bool eventFilter(QObject *watched, QEvent *event) override {
    if (overlay_ &&
        (event->type() == QEvent::Resize || event->type() == QEvent::Show ||
         event->type() == QEvent::EnabledChange)) {
      overlay_->reposition();
      overlay_->update();
    }
    return QObject::eventFilter(watched, event);
  }

private:
  ChevronOverlay *overlay_ = nullptr;
};

class FocusVisibleFilter : public QObject {
public:
  using QObject::QObject;

protected:
  bool eventFilter(QObject *watched, QEvent *event) override {
    auto *w = qobject_cast<QWidget *>(watched);
    if (!w)
      return false;

    const auto setKbd = [](QWidget *target, bool on) {
      if (!target || target->property("kbdFocus").toBool() == on)
        return;
      target->setProperty("kbdFocus", on);
      Theme::repolish(target);
    };

    if (event->type() == QEvent::FocusIn) {
      auto *fe = static_cast<QFocusEvent *>(event);
      if (fe->reason() == Qt::ActiveWindowFocusReason)
        return false;
      const bool kbd = (fe->reason() == Qt::TabFocusReason ||
                        fe->reason() == Qt::BacktabFocusReason ||
                        fe->reason() == Qt::ShortcutFocusReason);
      setKbd(w, kbd);
      if (QWidget *p = w->parentWidget()) {
        if (qobject_cast<QComboBox *>(p) || qobject_cast<QAbstractSpinBox *>(p))
          setKbd(p, kbd);
      }
    } else if (event->type() == QEvent::FocusOut) {
      auto *fe = static_cast<QFocusEvent *>(event);
      if (fe->reason() == Qt::ActiveWindowFocusReason)
        return false;
      setKbd(w, false);
      if (QWidget *p = w->parentWidget()) {
        if ((qobject_cast<QComboBox *>(p) ||
             qobject_cast<QAbstractSpinBox *>(p)) &&
            !p->hasFocus())
          setKbd(p, false);
      }
    }
    return false;
  }
};

class ThemeProxyStyle : public QProxyStyle {
public:
  explicit ThemeProxyStyle(QStyle *style)
      : QProxyStyle(style), focusFilter_(new FocusVisibleFilter(this)) {}

  void polish(QWidget *w) override {
    QProxyStyle::polish(w);
    if (w && !w->property("_semFocusHook").toBool()) {
      w->setProperty("_semFocusHook", true);
      w->installEventFilter(focusFilter_);
    }
    if (w && (w->inherits("QComboBox") || w->inherits("QDateEdit"))) {
      if (!w->property("_semChevronInstalled").toBool()) {
        w->setProperty("_semChevronInstalled", true);
        auto *overlay = new ChevronOverlay(w);
        w->installEventFilter(new ChevronResizeFilter(w, overlay));
      }
    }
  }

  void drawPrimitive(PrimitiveElement elem, const QStyleOption *opt,
                     QPainter *p, const QWidget *w) const override {
    if (elem == PE_IndicatorArrowDown || elem == PE_IndicatorArrowUp ||
        elem == PE_IndicatorHeaderArrow) {
      const bool enabled = opt->state & State_Enabled;
      bool up = (elem == PE_IndicatorArrowUp);
      if (elem == PE_IndicatorHeaderArrow) {
        if (const auto *hdr =
                qstyleoption_cast<const QStyleOptionHeader *>(opt))
          up = (hdr->sortIndicator == QStyleOptionHeader::SortUp);
      }
      p->save();
      p->setRenderHint(QPainter::Antialiasing, true);
      QPen pen(Theme::color(enabled ? "text-2" : "text-off"), 1.5,
               Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
      p->setPen(pen);
      p->setBrush(Qt::NoBrush);
      const QPointF c = QRectF(opt->rect).center();
      const qreal dx = (elem == PE_IndicatorHeaderArrow) ? 3.0 : 4.0;
      const qreal dy = (elem == PE_IndicatorHeaderArrow) ? 1.8 : 2.2;
      QPainterPath path;
      if (up) {
        path.moveTo(c.x() - dx, c.y() + dy * 0.6);
        path.lineTo(c.x(), c.y() - dy);
        path.lineTo(c.x() + dx, c.y() + dy * 0.6);
      } else {
        path.moveTo(c.x() - dx, c.y() - dy * 0.6);
        path.lineTo(c.x(), c.y() + dy);
        path.lineTo(c.x() + dx, c.y() - dy * 0.6);
      }
      p->drawPath(path);
      p->restore();
      return;
    }
    QProxyStyle::drawPrimitive(elem, opt, p, w);
  }

  QRect subElementRect(SubElement element, const QStyleOption *opt,
                       const QWidget *w) const override {
    if (element == SE_HeaderArrow) {
      const int sz = 10;
      return QRect(opt->rect.right() - sz - 6, opt->rect.center().y() - sz / 2,
                   sz, sz);
    }
    if (element == SE_HeaderLabel) {
      if (const auto *hdr =
              qstyleoption_cast<const QStyleOptionHeader *>(opt)) {
        if (hdr->sortIndicator != QStyleOptionHeader::None)
          return QProxyStyle::subElementRect(element, opt, w)
              .adjusted(0, 0, -16, 0);
      }
    }
    return QProxyStyle::subElementRect(element, opt, w);
  }

private:
  FocusVisibleFilter *focusFilter_ = nullptr;
};

QString expand(QString qss) {
  for (auto it = g_colors.constBegin(); it != g_colors.constEnd(); ++it)
    qss.replace(QStringLiteral("@{") + it.key() + QLatin1Char('}'),
                it.value().name());
  return qss;
}

} // namespace

namespace Theme {

Mode systemMode() {
  const QPalette p = QApplication::palette();
  return p.color(QPalette::Window).lightness() <
                 p.color(QPalette::WindowText).lightness()
             ? Mode::Dark
             : Mode::Light;
}

Mode mode() { return g_mode; }

bool systemHighContrast() {
#ifdef Q_OS_WIN
  HIGHCONTRASTW hc{};
  hc.cbSize = sizeof(hc);
  if (SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(hc), &hc, 0))
    return (hc.dwFlags & HCF_HIGHCONTRASTON) != 0;
#endif
  return false;
}

bool highContrast() { return g_highContrast; }

Contrast contrast() {
  return g_highContrast ? Contrast::High : Contrast::Standard;
}

void apply(QApplication &app, Mode m, Contrast c) {
  apply(app, m, c == Contrast::High);
}

void apply(QApplication &app, Mode m, bool hc) {
  g_mode = m;
  g_highContrast = hc;
  if (m == Mode::Dark) {
    if (hc)
      build(kDarkHcNeutral, kDarkHcAccent, kDarkHcSuccess, kDarkHcWarning,
            kDarkHcDanger, kDarkHcControl, kDarkHcTextOff, kOnSolid, true, true);
    else
      build(kDarkNeutral, kDarkAccent, kDarkSuccess, kDarkWarning, kDarkDanger,
            kDarkControl, kDarkTextOff, kOnSolid, true, false);
  } else {
    if (hc)
      build(kLightHcNeutral, kLightHcAccent, kLightHcSuccess, kLightHcWarning,
            kLightHcDanger, kLightHcControl, kLightHcTextOff, kOnSolid, false,
            true);
    else
      build(kLightNeutral, kLightAccent, kLightSuccess, kLightWarning,
            kLightDanger, kLightControl, kLightTextOff, kOnSolid, false, false);
  }

  static bool styled = false;
  if (!styled) {
    // Fusion gives every platform the same, themeable base to paint our roles
    // onto.
    if (QStyle *fusion = QStyleFactory::create(QStringLiteral("Fusion")))
      app.setStyle(new ThemeProxyStyle(fusion));
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
  font.setFamilies(QStringList{
      QStringLiteral("Inter"), QStringLiteral("Segoe UI"),
      QStringLiteral("SF Pro Text"), QStringLiteral("Helvetica Neue"),
      QStringLiteral("Noto Sans")});
#endif
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
  font.setFeature(QFont::Tag("tnum"), 1);
#endif
  app.setFont(font);

  app.setStyleSheet(expand(QString::fromLatin1(kQss)));
}

QColor color(const char *role) {
  return g_colors.value(QString::fromLatin1(role));
}

QFont titleFont(int pixelSize) {
  QFont f;
  f.setPixelSize(pixelSize);
  f.setWeight(QFont::DemiBold);
  f.setLetterSpacing(QFont::AbsoluteSpacing, pixelSize >= 40 ? -0.06 * pixelSize
                                             : pixelSize >= 24
                                                 ? -0.04 * pixelSize
                                                 : -0.02 * pixelSize);
  return f;
}

QFont monoFont(int pixelSize) {
  QFont f;
  f.setPixelSize(pixelSize);
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
  f.setFamilies(
      QStringList{QStringLiteral("JetBrains Mono"),
                  QStringLiteral("Cascadia Mono"), QStringLiteral("Consolas"),
                  QStringLiteral("SF Mono"), QStringLiteral("monospace")});
#else
  f.setStyleHint(QFont::Monospace);
#endif
  return f;
}

void repolish(QWidget *widget) {
  widget->style()->unpolish(widget);
  widget->style()->polish(widget);
  widget->update();
}

QString quoted(const QString &text) {
  return QString::fromUtf8("\xE2\x80\x9C") + text +
         QString::fromUtf8("\xE2\x80\x9D");
}

} // namespace Theme
