#include "nexus/ui/focus_manager.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <unordered_set>

namespace nexus::ui {

void FocusManager::set_targets(std::vector<FocusTarget> targets) {
    std::unordered_set<std::string> ids;
    for (const auto& target : targets) {
        if (target.id.empty() || target.column < 0 || target.row < 0 ||
            target.bounds.width <= 0 || target.bounds.height <= 0) {
            throw std::invalid_argument("Focus targets require an id and positive bounds.");
        }
        if (!ids.insert(target.id).second) {
            throw std::invalid_argument("Focus target ids must be unique.");
        }
    }

    const std::string previous_id(focused_id());
    targets_ = std::move(targets);
    focused_index_.reset();
    if (!previous_id.empty() && focus(previous_id)) {
        return;
    }
    if (!targets_.empty()) {
        focused_index_ = 0;
    }
}

std::string_view FocusManager::focused_id() const {
    if (!focused_index_ || *focused_index_ >= targets_.size()) {
        return {};
    }
    return targets_[*focused_index_].id;
}

bool FocusManager::focus(std::string_view id) {
    const auto found = std::find_if(targets_.begin(), targets_.end(), [id](const FocusTarget& target) {
        return target.id == id;
    });
    if (found == targets_.end()) {
        return false;
    }
    focused_index_ = static_cast<std::size_t>(std::distance(targets_.begin(), found));
    return true;
}

bool FocusManager::focus_at(int x, int y) {
    const auto found = std::find_if(targets_.begin(), targets_.end(), [x, y](const FocusTarget& target) {
        return x >= target.bounds.x && x < target.bounds.x + target.bounds.width &&
            y >= target.bounds.y && y < target.bounds.y + target.bounds.height;
    });
    if (found == targets_.end()) {
        return false;
    }
    focused_index_ = static_cast<std::size_t>(std::distance(targets_.begin(), found));
    return true;
}

bool FocusManager::move(Direction direction) {
    if (!focused_index_ || *focused_index_ >= targets_.size()) {
        return false;
    }

    const FocusTarget& current = targets_[*focused_index_];
    const FocusTarget* best = nullptr;
    int best_primary = 0;
    int best_secondary = 0;

    for (const auto& candidate : targets_) {
        const int column_delta = candidate.column - current.column;
        const int row_delta = candidate.row - current.row;
        int primary = 0;
        int secondary = 0;

        switch (direction) {
        case Direction::left:
            if (column_delta >= 0) continue;
            primary = -column_delta;
            secondary = std::abs(row_delta);
            break;
        case Direction::right:
            if (column_delta <= 0) continue;
            primary = column_delta;
            secondary = std::abs(row_delta);
            break;
        case Direction::up:
            if (row_delta >= 0) continue;
            primary = -row_delta;
            secondary = std::abs(column_delta);
            break;
        case Direction::down:
            if (row_delta <= 0) continue;
            primary = row_delta;
            secondary = std::abs(column_delta);
            break;
        }

        if (!best || secondary < best_secondary ||
            (secondary == best_secondary && primary < best_primary)) {
            best = &candidate;
            best_primary = primary;
            best_secondary = secondary;
        }
    }

    return best != nullptr && focus(best->id);
}

} // namespace nexus::ui
