#include "CommandRegistry.h"
#include <QAction>
#include <QMenu>
#include <QWidget>
#include <algorithm>

CommandRegistry::CommandRegistry(QObject* parent) : QObject(parent) {}

void CommandRegistry::registerCommand(CommandSpec spec) {
    entries_.push_back(Entry{std::move(spec), nullptr});
}

int CommandRegistry::indexOf(const QString& id) const {
    for (int i = 0; i < entries_.size(); ++i) {
        if (entries_[i].spec.id == id) return i;
    }
    return -1;
}

QAction* CommandRegistry::bindAction(const QString& id, QWidget* shortcutOwner, QMenu* menu) {
    const int idx = indexOf(id);
    if (idx < 0) return nullptr;
    Entry& entry = entries_[idx];
    if (!entry.action) {
        entry.action = new QAction(entry.spec.title, shortcutOwner);
        if (!entry.spec.shortcut.isEmpty()) {
            entry.action->setShortcut(entry.spec.shortcut);
            entry.action->setShortcutContext(Qt::WindowShortcut);
        }
        entry.action->setCheckable(entry.spec.checkable);
        if (entry.spec.checkable && entry.spec.isChecked) entry.action->setChecked(entry.spec.isChecked());
        connect(entry.action, &QAction::triggered, this, [this, id] { trigger(id); });
        if (shortcutOwner) shortcutOwner->addAction(entry.action);
        if (menu) menu->addAction(entry.action);
    } else if (menu) {
        menu->addAction(entry.action);
    }
    return entry.action;
}

QAction* CommandRegistry::actionFor(const QString& id) const {
    const int idx = indexOf(id);
    return idx >= 0 ? entries_[idx].action : nullptr;
}

void CommandRegistry::syncActions() {
    for (const Entry& entry : entries_) {
        if (!entry.action) continue;
        const bool enabled = !entry.spec.isEnabled || entry.spec.isEnabled();
        entry.action->setEnabled(enabled);
        if (entry.spec.checkable && entry.spec.isChecked) {
            entry.action->blockSignals(true);
            entry.action->setChecked(entry.spec.isChecked());
            entry.action->blockSignals(false);
        }
    }
}

const CommandSpec* CommandRegistry::find(const QString& id) const {
    const int idx = indexOf(id);
    return idx >= 0 ? &entries_[idx].spec : nullptr;
}

bool CommandRegistry::trigger(const QString& id) const {
    const CommandSpec* spec = find(id);
    if (!spec) return false;
    if (spec->isEnabled && !spec->isEnabled()) return false;
    if (spec->trigger) spec->trigger();
    return true;
}

int CommandRegistry::fuzzyScore(const QString& query, const QString& candidate) {
    const QString q = query.trimmed().toLower();
    if (q.isEmpty()) return 0;
    const QString c = candidate.toLower();

    int qi = 0;
    int score = 0;
    int prevMatch = -2;
    for (int ci = 0; ci < c.size() && qi < q.size(); ++ci) {
        if (c[ci] == q[qi]) {
            score += 10;
            if (ci == 0) score += 35;
            else if (!c[ci - 1].isLetterOrNumber()) score += 25;
            if (ci == prevMatch + 1) score += 18;
            prevMatch = ci;
            ++qi;
        }
    }
    if (qi < q.size()) return -1;
    score -= (c.size() - q.size()) / 3;
    return score;
}

QVector<CommandMatch> CommandRegistry::search(const QString& query) const {
    const QString trimmed = query.trimmed();
    QVector<CommandMatch> matches;
    for (int i = 0; i < entries_.size(); ++i) {
        const CommandSpec& spec = entries_[i].spec;
        const bool enabled = !spec.isEnabled || spec.isEnabled();

        int boost = spec.contextBoost ? spec.contextBoost() : 0;
        const int availBoost = enabled ? 200 : 0;
        if (trimmed.isEmpty()) {
            matches.push_back(CommandMatch{&spec, availBoost + 100 + boost - i, QString(), enabled});
            continue;
        }

        int best = fuzzyScore(trimmed, spec.title);
        QString matchedAlias;
        const int catScore = fuzzyScore(trimmed, spec.category + QLatin1Char(' ') + spec.title);
        if (catScore > best) best = catScore;

        for (const QString& alias : spec.aliases) {
            const int aliasScore = fuzzyScore(trimmed, alias);
            if (aliasScore > best) {
                best = aliasScore;
                matchedAlias = alias;
            }
        }
        if (best >= 0) matches.push_back(CommandMatch{&spec, availBoost + best + boost, matchedAlias, enabled});
    }

    std::stable_sort(matches.begin(), matches.end(), [](const CommandMatch& a, const CommandMatch& b) {
        return a.score > b.score;
    });
    return matches;
}
