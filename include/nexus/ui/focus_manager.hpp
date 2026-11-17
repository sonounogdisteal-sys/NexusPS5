#pragma once

#include "nexus/ui/canvas.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nexus::ui {

enum class Direction {
    left,
    right,
    up,
    down,
};

struct FocusTarget {
    std::string id;
    int column;
    int row;
    Rect bounds;
};

class FocusManager {
public:
    void set_targets(std::vector<FocusTarget> targets);

    [[nodiscard]] std::string_view focused_id() const;
    [[nodiscard]] bool focus(std::string_view id);
    [[nodiscard]] bool focus_at(int x, int y);
    [[nodiscard]] bool move(Direction direction);

private:
    std::vector<FocusTarget> targets_;
    std::optional<std::size_t> focused_index_;
};

} // namespace nexus::ui
