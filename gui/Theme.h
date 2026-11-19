#pragma once
#include <QColor>
#include <QFont>
#include <QString>

class QApplication;
class QWidget;

// Design tokens and the application stylesheet. See docs/DESIGN.md for the rules behind them.
namespace Theme {

enum class Mode { Light, Dark };

// Spacing: 4px base grid.
constexpr int space1 = 4;
constexpr int space2 = 8;
constexpr int space3 = 12;
constexpr int space4 = 16;
constexpr int space6 = 24;

// Shape: one radius family (controls 6, surfaces 12).
constexpr int radiusControl = 6;
constexpr int radiusSurface = 12;

constexpr int rowHeight = 36;
constexpr int cellPadding = 12;

Mode systemMode();
Mode mode();

// Builds the colour roles for `mode`, then applies palette, font and stylesheet to the application.
void apply(QApplication& app, Mode mode);

// Role lookup, e.g. color("text"), color("accent-bg"). Unknown roles return an invalid colour.
QColor color(const char* role);

// Heading font: semibold, tracking tightened as size grows.
QFont titleFont(int pixelSize = 20);

// Re-evaluates the stylesheet after a dynamic property (variant, invalid, chip, state) changed.
void repolish(QWidget* widget);

// “text” with typographic quotes.
QString quoted(const QString& text);

}  // namespace Theme
