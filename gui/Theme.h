#pragma once
#include <QColor>
#include <QFont>
#include <QString>

class QApplication;
class QWidget;

// Design tokens and the application stylesheet. See docs/DESIGN.md for the rules behind them.
namespace Theme {

enum class Mode { Light, Dark };
enum class Contrast { Standard, High };

// Spacing: 4px base grid.
constexpr int space1 = 4;
constexpr int space2 = 8;
constexpr int space3 = 12;
constexpr int space4 = 16;
constexpr int space6 = 24;
constexpr int space8 = 32;
constexpr int space10 = 40;

// Shape: one radius family (controls 6, surfaces 12).
constexpr int radiusControl = 6;
constexpr int radiusSurface = 12;

constexpr int rowHeight = 40;
constexpr int controlHeight = 34;
constexpr int cellPadding = 12;

Mode systemMode();
Mode mode();
bool systemHighContrast();
Contrast contrast();
bool highContrast();

// Builds the colour roles for `mode` and `contrast`, then applies palette, font and stylesheet to the application.
void apply(QApplication& app, Mode mode, Contrast contrast = Contrast::Standard);
void apply(QApplication& app, Mode mode, bool highContrast);

// Role lookup, e.g. color("text"), color("accent-bg"). Unknown roles return an invalid colour.
QColor color(const char* role);

// Heading font: semibold, tracking tightened as size grows.
QFont titleFont(int pixelSize = 20);
// Monospace font for IDs, tabular counters, and shortcut keycaps.
QFont monoFont(int pixelSize = 12);

// Re-evaluates the stylesheet after a dynamic property (variant, invalid, chip, state) changed.
void repolish(QWidget* widget);

// “text” with typographic quotes.
QString quoted(const QString& text);

}  // namespace Theme
