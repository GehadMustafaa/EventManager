#include "Delegates.h"
#include <QFontMetrics>
#include <QPainter>
#include <QtGlobal>
#include "Theme.h"

namespace {
constexpr int kMeterWidth = 56;
constexpr int kMeterHeight = 4;
constexpr int kMeterGap = 10;
constexpr int kPositionColumn = 28;
constexpr int kCancelWidth = 64;
constexpr int kCancelHeight = 26;
}

void HoverDelegate::paintBackground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    const bool selected = option.state & QStyle::State_Selected;
    if (selected) {
        painter->fillRect(option.rect, Theme::color("accent-bg"));
        if (index.column() == 0) {
            const bool focused = option.state & QStyle::State_HasFocus;
            painter->fillRect(QRect(option.rect.left(), option.rect.top() + 4, 3, option.rect.height() - 8),
                              Theme::color(focused ? "accent" : "accent-text"));
        }
        painter->save();
        painter->setPen(Theme::color("accent-border"));
        painter->drawLine(option.rect.topLeft(), option.rect.topRight());
        painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());
        painter->restore();
    } else if (index.row() == hoverRow) {
        painter->fillRect(option.rect, Theme::color("fill"));
    }
}

void TableDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    painter->save();
    paintBackground(painter, option, index);

    const QRect area = option.rect.adjusted(Theme::cellPadding, 0, -Theme::cellPadding, 0);
    const bool secondary = index.data(ItemRole::Tone).toInt() == 1;

    if (index.data(ItemRole::Kind).toInt() == ItemRole::Seats) {
        const int capacity = index.data(ItemRole::Capacity).toInt();
        const int booked = index.data(ItemRole::Booked).toInt();
        const bool full = capacity > 0 && booked >= capacity;
        const double fraction = capacity > 0 ? double(booked) / capacity : 0.0;

        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(Qt::NoPen);
        const QRect track(area.left(), area.center().y() - kMeterHeight / 2, kMeterWidth, kMeterHeight);
        painter->setBrush(Theme::color("fill-active"));
        painter->drawRoundedRect(track, 2, 2);

        const int fillWidth = booked > 0 ? qMax(kMeterHeight, int(kMeterWidth * fraction)) : 0;
        painter->setBrush(Theme::color(full ? "danger" : fraction >= 0.8 ? "warning" : "accent"));
        if (fillWidth > 0) painter->drawRoundedRect(QRect(track.left(), track.top(), qMin(fillWidth, kMeterWidth), kMeterHeight), 2, 2);

        // State is also spelled out in text, so colour is never the only signal.
        const QString text = full ? QStringLiteral("Full") : QStringLiteral("%1 / %2").arg(booked).arg(capacity);
        const QRect textRect(track.right() + kMeterGap, area.top(), area.right() - track.right() - kMeterGap, area.height());
        painter->setPen(Theme::color(full ? "danger-text" : "text-2"));
        painter->drawText(textRect, int(Qt::AlignVCenter) | int(Qt::AlignLeft), option.fontMetrics.elidedText(text, Qt::ElideRight, textRect.width()));
    } else {
        const bool right = index.data(ItemRole::AlignRight).toBool();
        const bool mono = index.data(ItemRole::Monospace).toBool();
        if (mono) painter->setFont(Theme::monoFont(12));
        const QVariant elideVar = index.data(ItemRole::ElideMode);
        const Qt::TextElideMode elideMode =
            elideVar.isValid() ? static_cast<Qt::TextElideMode>(elideVar.toInt()) : Qt::ElideRight;
        const QFontMetrics fm = painter->fontMetrics();
        const QString text = fm.elidedText(index.data(Qt::DisplayRole).toString(), elideMode, area.width());
        painter->setPen(Theme::color(secondary ? "text-2" : "text"));
        painter->drawText(area, int(Qt::AlignVCenter) | int(right ? Qt::AlignRight : Qt::AlignLeft), text);
    }
    painter->restore();
}

QSize TableDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    Q_UNUSED(index);
    return QSize(option.rect.width(), Theme::rowHeight);
}

QRect PersonDelegate::cancelRect(const QRect& rowRect) {
    const QRect area = rowRect.adjusted(Theme::cellPadding, 0, -Theme::cellPadding, 0);
    return QRect(area.right() - kCancelWidth, area.center().y() - kCancelHeight / 2, kCancelWidth, kCancelHeight);
}

void PersonDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    const bool selected = option.state & QStyle::State_Selected;
    if (selected || index.row() == hoverRow) {
        const QRect bgRect = option.rect.adjusted(2, 1, -2, -1);
        painter->setPen(Qt::NoPen);
        painter->setBrush(Theme::color(selected ? "accent-bg" : "fill"));
        painter->drawRoundedRect(bgRect, Theme::radiusControl, Theme::radiusControl);
        if (selected && (option.state & QStyle::State_HasFocus)) {
            painter->setBrush(Theme::color("accent"));
            painter->drawRoundedRect(QRect(bgRect.left() + 2, bgRect.top() + 5, 3, bgRect.height() - 10), 1, 1);
        }
    }

    QRect area = option.rect.adjusted(Theme::cellPadding, 0, -Theme::cellPadding, 0);
    const bool showCancel = (index.row() == hoverRow) && index.data(ItemRole::ShowCancelAction).toBool();
    if (showCancel) {
        const QRect btn = cancelRect(option.rect);
        painter->setPen(Theme::color("danger-border"));
        painter->setBrush(Theme::color("danger-bg"));
        painter->drawRoundedRect(btn, Theme::radiusControl, Theme::radiusControl);
        QFont btnFont = option.font;
        btnFont.setPixelSize(12);
        btnFont.setWeight(QFont::DemiBold);
        painter->setFont(btnFont);
        painter->setPen(Theme::color("danger-text"));
        painter->drawText(btn, Qt::AlignCenter, QStringLiteral("Remove"));
        area.setRight(btn.left() - Theme::space2);
    }

    const QString badgeText = index.data(ItemRole::BadgeText).toString();
    if (!badgeText.isEmpty()) {
        QFont badgeFont = option.font;
        badgeFont.setPixelSize(11);
        badgeFont.setWeight(QFont::DemiBold);
        const QFontMetrics bfm(badgeFont);
        const int bw = bfm.horizontalAdvance(badgeText) + 14;
        const int bh = 20;
        if (area.width() > bw + 60) {
            const QRect bRect(area.right() - bw, area.center().y() - bh / 2, bw, bh);
            const QString kind = index.data(ItemRole::BadgeKind).toString();
            const QString bgKey = kind == "warning" ? "warning-bg" : kind == "neutral" ? "fill" : "success-bg";
            const QString fgKey = kind == "warning" ? "warning-text" : kind == "neutral" ? "text-2" : "success-text";
            painter->setRenderHint(QPainter::Antialiasing, true);
            painter->setPen(Qt::NoPen);
            painter->setBrush(Theme::color(bgKey.toLatin1().constData()));
            painter->drawRoundedRect(bRect, 10, 10);
            painter->setFont(badgeFont);
            painter->setPen(Theme::color(fgKey.toLatin1().constData()));
            painter->drawText(bRect, Qt::AlignCenter, badgeText);
            area.setRight(bRect.left() - Theme::space2);
        }
    }

    painter->setFont(option.font);
    const int position = index.data(ItemRole::Position).toInt();
    if (position > 0) {
        painter->setFont(Theme::monoFont(12));
        painter->setPen(Theme::color("text-2"));
        painter->drawText(QRect(area.left(), area.top(), kPositionColumn, area.height()),
                          int(Qt::AlignVCenter) | int(Qt::AlignLeft), QString::number(position));
        area.setLeft(area.left() + kPositionColumn);
        painter->setFont(option.font);
    }

    const QFontMetrics& fm = option.fontMetrics;
    const QString name = index.data(Qt::DisplayRole).toString();
    const QString email = index.data(ItemRole::Email).toString();
    const QVariant elideVar = index.data(ItemRole::ElideMode);
    const Qt::TextElideMode elideMode =
        elideVar.isValid() ? static_cast<Qt::TextElideMode>(elideVar.toInt()) : Qt::ElideRight;

    // Grant +6px rasterization slack so DirectWrite subpixel rounding never truncates fitting labels.
    constexpr int kTextSlack = 6;
    const int nameNatural = fm.horizontalAdvance(name) + kTextSlack;
    const int emailNatural = email.isEmpty() ? 0 : fm.horizontalAdvance(email) + kTextSlack;
    const int minEmailReserve = qMin(emailNatural, area.width() * 4 / 10);
    const int availForName = email.isEmpty() ? area.width()
                                             : qMax(area.width() * 5 / 10, area.width() - Theme::space2 - minEmailReserve);
    const int nameWidth = qMin(nameNatural, availForName);
    const QString drawnName = nameNatural > availForName ? fm.elidedText(name, elideMode, nameWidth) : name;
    painter->setPen(Theme::color("text"));
    painter->drawText(QRect(area.left(), area.top(), nameWidth, area.height()),
                      int(Qt::AlignVCenter) | int(Qt::AlignLeft), drawnName);

    if (!email.isEmpty()) {
        const int emailLeft = area.left() + nameWidth + Theme::space2;
        const int emailWidth = area.right() - emailLeft;
        if (emailWidth > 24) {
            const QString drawnEmail =
                emailNatural > emailWidth ? fm.elidedText(email, Qt::ElideRight, emailWidth) : email;
            painter->setPen(Theme::color("text-2"));
            painter->drawText(QRect(emailLeft, area.top(), emailWidth, area.height()),
                              int(Qt::AlignVCenter) | int(Qt::AlignLeft), drawnEmail);
        }
    }
    painter->restore();
}

QSize PersonDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    Q_UNUSED(index);
    return QSize(option.rect.width(), Theme::rowHeight);
}
