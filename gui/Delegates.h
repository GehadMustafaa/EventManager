#pragma once
#include <QStyledItemDelegate>

namespace ItemRole {
enum {
    Tone = Qt::UserRole + 1,  // int: 0 primary text, 1 secondary text
    Kind,                     // int: Kind::Plain or Kind::Seats
    AlignRight,               // bool
    Booked,                   // int, Kind::Seats only
    Capacity,                 // int, Kind::Seats only
    Email,                    // QString, person rows
    Position,                 // int, waitlist rows (1-based), 0 = none
    SortValue,                // qint64, optional numeric sort key
    Monospace,                // bool, render cell with Theme::monoFont
    BadgeText,                // QString, status pill (Confirmed / Waitlist #N)
    BadgeKind,                // QString, "success" | "warning" | "neutral"
    ShowCancelAction,         // bool, render hover Cancel chip on person/booking row
    ElideMode                 // int (Qt::TextElideMode), defaults to Qt::ElideRight
};
enum Kind { Plain = 0, Seats = 1 };
}  // namespace ItemRole

// Paints the row background (hover / selected) so rows read as one band across all columns.
class HoverDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    int hoverRow = -1;

protected:
    void paintBackground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
};

// Table cells: plain text in primary/secondary tone, or a seat meter with a text label.
class TableDelegate : public HoverDelegate {
public:
    using HoverDelegate::HoverDelegate;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

// List rows for people or bookings: optional waitlist position, name, secondary email/meta, badge, and hover Cancel chip.
class PersonDelegate : public HoverDelegate {
public:
    using HoverDelegate::HoverDelegate;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    static QRect cancelRect(const QRect& rowRect);
};
