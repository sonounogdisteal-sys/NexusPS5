#pragma once

#include "nexus/ui/canvas.hpp"

namespace nexus::ui {

enum class IconKind {
    home,
    library,
    profile,
    settings,
};

class Icon {
public:
    explicit Icon(IconKind kind);
    void render(Canvas& canvas, Rect bounds, Color color) const;

private:
    IconKind kind_;
};

} // namespace nexus::ui
