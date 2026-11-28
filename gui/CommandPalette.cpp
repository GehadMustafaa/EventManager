#include "CommandPalette.h"
#include <QApplication>
#include <QEvent>
#include <QFontMetrics>
#include <QFrame>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QVBoxLayout>
#include "CommandRegistry.h"
#include "Theme.h"

namespace {

enum PaletteRole {
    CategoryRole = Qt::UserRole + 1,
    AliasRole,
    ShortcutRole,
    HoverRole,
    EnabledRole
};

class CommandRowDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    int hoverRow = -1;

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
        painter->save();
        const QVariant enabledVar = index.data(EnabledRole);
        const bool enabled = !enabledVar.isValid() || enabledVar.toBool();
        const bool selected = option.state & QStyle::State_Selected;
        if (selected) {
            painter->fillRect(option.rect, enabled ? Theme::color("accent-bg") : Theme::color("fill"));
            painter->fillRect(QRect(option.rect.left(), option.rect.top() + 6, 3, option.rect.height() - 12),
                              enabled ? Theme::color("accent") : Theme::color("border-strong"));
        } else if (index.row() == hoverRow) {
            painter->fillRect(option.rect, Theme::color("fill"));
        }

        const QRect area = option.rect.adjusted(Theme::space4, 0, -Theme::space4, 0);
        const QString shortcut = index.data(ShortcutRole).toString();
        int rightEdge = area.right();

        if (!shortcut.isEmpty()) {
            const QFont kbdFont = Theme::monoFont(11);
            const QFontMetrics kbdFm(kbdFont);
            const int pillW = kbdFm.horizontalAdvance(shortcut) + 14;
            const int pillH = 22;
            const QRect pillRect(rightEdge - pillW, area.center().y() - pillH / 2, pillW, pillH);
            painter->setRenderHint(QPainter::Antialiasing, true);
            painter->setPen(Theme::color("border-subtle"));
            painter->setBrush(Theme::color("fill"));
            painter->drawRoundedRect(pillRect, 4, 4);
            painter->setFont(kbdFont);
            painter->setPen(Theme::color("text-2"));
            painter->drawText(pillRect, Qt::AlignCenter, shortcut);
            rightEdge = pillRect.left() - Theme::space3;
        }

        if (!enabled) {
            const QString tag = QStringLiteral("Unavailable");
            QFont tagFont = option.font;
            tagFont.setPixelSize(11);
            const QFontMetrics tagFm(tagFont);
            const int tagW = tagFm.horizontalAdvance(tag) + 12;
            const int tagH = 20;
            const QRect tagRect(rightEdge - tagW, area.center().y() - tagH / 2, tagW, tagH);
            painter->setRenderHint(QPainter::Antialiasing, true);
            QPen dashPen(Theme::color("border"));
            dashPen.setStyle(Qt::DashLine);
            painter->setPen(dashPen);
            painter->setBrush(Theme::color("fill"));
            painter->drawRoundedRect(tagRect, 4, 4);
            painter->setFont(tagFont);
            painter->setPen(Theme::color("text-2"));
            painter->drawText(tagRect, Qt::AlignCenter, tag);
            rightEdge = tagRect.left() - Theme::space2;
        }

        const QString category = index.data(CategoryRole).toString();
        const QString title = index.data(Qt::DisplayRole).toString();
        const QString alias = index.data(AliasRole).toString();

        painter->setFont(option.font);
        const QFontMetrics fm = option.fontMetrics;

        int x = area.left();
        if (!category.isEmpty()) {
            const QString catLabel = category + QStringLiteral("  ·  ");
            const int catW = fm.horizontalAdvance(catLabel);
            painter->setPen(Theme::color("text-2"));
            painter->drawText(QRect(x, area.top(), catW, area.height()), int(Qt::AlignVCenter) | int(Qt::AlignLeft), catLabel);
            x += catW;
        }

        const int avail = qMax(40, rightEdge - x);
        const QString titleText = alias.isEmpty() ? title : QString("%1  (%2)").arg(title, alias);
        painter->setPen(enabled ? Theme::color("text") : Theme::color("text-2"));
        painter->drawText(QRect(x, area.top(), avail, area.height()), int(Qt::AlignVCenter) | int(Qt::AlignLeft),
                          fm.elidedText(titleText, Qt::ElideRight, avail));
        painter->restore();
    }

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex&) const override {
        return QSize(option.rect.width(), 42);
    }
};

}  // namespace

CommandPalette::CommandPalette(CommandRegistry& registry, QWidget* parent)
    : QDialog(parent, Qt::Popup | Qt::FramelessWindowHint), registry_(registry) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    setModal(true);
    setFixedWidth(560);

    auto* frame = new QFrame;
    frame->setObjectName("commandPalette");

    input_ = new QLineEdit;
    input_->setObjectName("paletteInput");
    input_->setPlaceholderText(QString::fromUtf8("Type a command or jump to an event or attendee\xE2\x80\xA6"));
    input_->installEventFilter(this);

    list_ = new QListWidget;
    list_->setFrameShape(QFrame::NoFrame);
    list_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    list_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    list_->setMouseTracking(true);
    list_->setMinimumHeight(210);
    // 312px cleanly shows ~7 rows with the last row subtly clipped to signal scrollability.
    list_->setMaximumHeight(312);
    auto* delegate = new CommandRowDelegate(list_);
    list_->setItemDelegate(delegate);
    connect(list_, &QAbstractItemView::entered, this, [this, delegate](const QModelIndex& idx) {
        delegate->hoverRow = idx.row();
        list_->viewport()->update();
    });
    connect(list_, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        list_->setCurrentItem(item);
        activateCurrent();
    });

    auto* footer = new QWidget;
    auto* footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(Theme::space4, Theme::space2, Theme::space4, Theme::space2);
    footerLayout->setSpacing(Theme::space2);
    auto* hint = new QLabel("Up/Down navigate  ·  Enter run  ·  Esc close");
    hint->setProperty("role", "caption");
    footerLayout->addWidget(hint);
    footerLayout->addStretch();

    auto* frameLayout = new QVBoxLayout(frame);
    frameLayout->setContentsMargins(0, 0, 0, 0);
    frameLayout->setSpacing(0);
    frameLayout->addWidget(input_);
    frameLayout->addWidget(list_, 1);
    frameLayout->addWidget(footer);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->addWidget(frame);

    connect(input_, &QLineEdit::textChanged, this, &CommandPalette::populate);
}

void CommandPalette::setJumpProvider(JumpProvider provider) {
    jumpProvider_ = std::move(provider);
}

void CommandPalette::openPalette() {
    if (isVisible()) {
        reject();
        return;
    }
    previousFocus_ = QApplication::focusWidget();
    input_->clear();
    populate(QString());

    if (parentWidget()) {
        const QRect parentRect = parentWidget()->frameGeometry();
        const int x = parentRect.center().x() - width() / 2;
        const int y = parentRect.top() + qMax(64, parentRect.height() / 6);
        move(x, y);
    }
    show();
    input_->setFocus(Qt::ShortcutFocusReason);
}

void CommandPalette::populate(const QString& query) {
    list_->clear();
    rowCallbacks_.clear();

    const QVector<CommandMatch> matches = registry_.search(query);
    for (const CommandMatch& m : matches) {
        auto* item = new QListWidgetItem(m.spec->title);
        item->setData(CategoryRole, m.spec->category);
        item->setData(AliasRole, m.matchedAlias);
        const QString sc = m.spec->shortcut.toString(QKeySequence::NativeText);
        item->setData(ShortcutRole, sc);
        item->setData(EnabledRole, m.enabled);
        list_->addItem(item);

        const QString id = m.spec->id;
        const QString title = m.spec->title;
        const bool enabled = m.enabled;
        rowCallbacks_.push_back([this, enabled, id, title, sc] {
            if (!enabled) return;
            accept();
            registry_.trigger(id);
            if (!sc.isEmpty()) emit commandExecuted(title, sc);
        });
    }

    if (jumpProvider_ && !query.trimmed().isEmpty()) {
        const QVector<PaletteJumpItem> jumps = jumpProvider_(query);
        for (const PaletteJumpItem& j : jumps) {
            auto* item = new QListWidgetItem(j.title);
            item->setData(CategoryRole, j.category);
            item->setData(AliasRole, j.subtitle);
            list_->addItem(item);
            auto cb = j.trigger;
            rowCallbacks_.push_back([this, cb] {
                accept();
                if (cb) cb();
            });
        }
    }

    if (list_->count() == 0) {
        auto* empty = new QListWidgetItem(QString("No commands match “%1”").arg(query.trimmed()));
        empty->setFlags(Qt::NoItemFlags);
        list_->addItem(empty);
        rowCallbacks_.push_back(nullptr);
    } else {
        list_->setCurrentRow(0);
    }
}

void CommandPalette::activateCurrent() {
    const int row = list_->currentRow();
    if (row < 0 || row >= rowCallbacks_.size() || !rowCallbacks_[row]) return;
    rowCallbacks_[row]();
}

void CommandPalette::moveSelection(int delta) {
    if (list_->count() == 0) return;
    int next = qBound(0, list_->currentRow() + delta, list_->count() - 1);
    list_->setCurrentRow(next);
}

bool CommandPalette::eventFilter(QObject* watched, QEvent* event) {
    if (watched == input_ && event->type() == QEvent::KeyPress) {
        auto* ke = static_cast<QKeyEvent*>(event);
        if (ke->key() == Qt::Key_Escape) {
            reject();
            return true;
        }
        if ((ke->modifiers() & Qt::ControlModifier) && ke->key() == Qt::Key_K) {
            reject();
            return true;
        }
        if (ke->key() == Qt::Key_Down) {
            moveSelection(1);
            return true;
        }
        if (ke->key() == Qt::Key_Up) {
            moveSelection(-1);
            return true;
        }
        if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
            activateCurrent();
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}

void CommandPalette::hideEvent(QHideEvent* event) {
    QDialog::hideEvent(event);
    restoreFocus();
}

void CommandPalette::restoreFocus() {
    if (previousFocus_ && previousFocus_->isVisible()) {
        previousFocus_->setFocus(Qt::OtherFocusReason);
    }
    previousFocus_.clear();
}
