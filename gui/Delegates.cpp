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
}

void HoverDelegate::paintBackground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    const bool selected = option.state & QStyle::State_Selected;
    if (selected) painter->fillRect(option.rect, Theme::color("accent-bg"));
    else if (index.row() == hoverRow) painter->fillRect(option.rect, Theme::color("fill"));
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
        const QString text = option.fontMetrics.elidedText(index.data(Qt::DisplayRole).toString(), Qt::ElideRight, area.width());
        painter->setPen(Theme::color(secondary ? "text-2" : "text"));
        painter->drawText(area, int(Qt::AlignVCenter) | int(right ? Qt::AlignRight : Qt::AlignLeft), text);
    }
    painter->restore();
}

QSize TableDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    Q_UNUSED(index);
    return QSize(option.rect.width(), Theme::rowHeight);
}

void PersonDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    painter->save();
    paintBackground(painter, option, index);

    QRect area = option.rect.adjusted(Theme::cellPadding, 0, -Theme::cellPadding, 0);
    const int position = index.data(ItemRole::Position).toInt();
    if (position > 0) {
        painter->setPen(Theme::color("text-2"));
        painter->drawText(QRect(area.left(), area.top(), kPositionColumn, area.height()),
                          int(Qt::AlignVCenter) | int(Qt::AlignLeft), QString::number(position));
        area.setLeft(area.left() + kPositionColumn);
    }

    const QFontMetrics& fm = option.fontMetrics;
    const QString name = index.data(Qt::DisplayRole).toString();
    const QString email = index.data(ItemRole::Email).toString();

    const int nameWidth = qMin(fm.horizontalAdvance(name), area.width() * 6 / 10);
    painter->setPen(Theme::color("text"));
    painter->drawText(QRect(area.left(), area.top(), nameWidth, area.height()), int(Qt::AlignVCenter) | int(Qt::AlignLeft),
                      fm.elidedText(name, Qt::ElideRight, nameWidth));

    const int emailLeft = area.left() + nameWidth + Theme::space2;
    const int emailWidth = area.right() - emailLeft;
    if (emailWidth > 24) {
        painter->setPen(Theme::color("text-2"));
        painter->drawText(QRect(emailLeft, area.top(), emailWidth, area.height()), int(Qt::AlignVCenter) | int(Qt::AlignLeft),
                          fm.elidedText(email, Qt::ElideRight, emailWidth));
    }
    painter->restore();
}

QSize PersonDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    Q_UNUSED(index);
    return QSize(option.rect.width(), Theme::rowHeight);
}
