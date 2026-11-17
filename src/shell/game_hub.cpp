#include "nexus/shell/game_hub.hpp"

#include "nexus/ui/theme.hpp"

#include <array>
#include <string_view>
#include <utility>

namespace nexus::shell {
namespace {

constexpr std::array<std::string_view, 4> tabs{
    "Overview", "Activities", "Trophies", "Related",
};

} // namespace

void GameHubScreen::set_game(std::string title, std::string description, ui::Color artwork) {
    title_ = std::move(title);
    description_ = std::move(description);
    artwork_ = artwork;
    selected_tab_ = 0;
    status_message_.clear();
}

bool GameHubScreen::handle(input::Action action) {
    using input::Action;
    if (action == Action::move_left) {
        selected_tab_ = (selected_tab_ + static_cast<int>(tabs.size()) - 1) %
            static_cast<int>(tabs.size());
        return true;
    }
    if (action == Action::move_right) {
        selected_tab_ = (selected_tab_ + 1) % static_cast<int>(tabs.size());
        return true;
    }
    if (action == Action::accept) {
        if (selected_tab_ == 0) {
            status_message_ = "Guest execution is unavailable; this is local test data only.";
        } else {
            status_message_ = "No online account or game-service data is connected.";
        }
        return true;
    }
    return action == Action::move_up || action == Action::move_down;
}

void GameHubScreen::mouse_move(int x, int y) {
    if (y >= 230 && y < 296 && x >= 356) {
        const int tab = (x - 356) / 176;
        if (tab >= 0 && tab < static_cast<int>(tabs.size())) {
            selected_tab_ = tab;
        }
    }
}

bool GameHubScreen::mouse_click(int x, int y) {
    if (y >= 230 && y < 296 && x >= 356) {
        mouse_move(x, y);
        return true;
    }
    if (y >= 304 && y < 520 && x >= 356 && x < 1202) {
        status_message_ = selected_tab_ == 0
            ? "Guest execution is unavailable; this is local test data only."
            : "No online account or game-service data is connected.";
        return true;
    }
    return false;
}

void GameHubScreen::render(ui::Canvas& canvas, bool high_contrast) const {
    const ui::Color background = high_contrast
        ? ui::theme::high_contrast_background
        : ui::theme::background;
    const ui::Color foreground = high_contrast
        ? ui::theme::high_contrast_text
        : ui::theme::text;
    const ui::Color subdued = high_contrast
        ? ui::theme::high_contrast_text
        : ui::theme::subdued_text;
    const ui::Color panel = high_contrast ? ui::Color{0, 0, 0, 255} : ui::theme::panel;
    canvas.fill_rect({0, 0, 1280, 720}, background);
    canvas.fill_rect({0, 0, 1280, 8}, artwork_);
    canvas.fill_rect({62, 116, 254, 348}, artwork_);
    canvas.fill_rect(
        {86, 140, 206, 300},
        {static_cast<std::uint8_t>(artwork_.red / 2),
         static_cast<std::uint8_t>(artwork_.green / 2),
         static_cast<std::uint8_t>(artwork_.blue / 2), 255}
    );
    canvas.draw_text("LOCAL TEST ART", 104, 264, 15, foreground);
    canvas.draw_text("GAME HUB  |  LOCAL TEST DATA", 62, 42, 14, subdued);
    canvas.draw_text(title_, 356, 126, 35, foreground);
    canvas.draw_text(description_, 358, 177, 17, subdued);

    for (std::size_t index = 0; index < tabs.size(); ++index) {
        const int x = 356 + static_cast<int>(index) * 176;
        const ui::Color color = static_cast<int>(index) == selected_tab_
            ? foreground
            : subdued;
        canvas.draw_text(tabs[index], x, 246, 17, color);
        if (static_cast<int>(index) == selected_tab_) {
            canvas.fill_rect(
                {x, 276, 120, 3},
                high_contrast ? ui::theme::high_contrast_text : ui::theme::focus
            );
        }
    }

    canvas.fill_rect({356, 304, 846, 216}, panel);
    switch (selected_tab_) {
    case 0:
        canvas.draw_text("Play", 386, 332, 22, foreground);
        canvas.draw_text("Launching guest applications is not implemented.", 386, 383, 16, subdued);
        canvas.draw_text("Activities", 386, 434, 19, foreground);
        canvas.draw_text("Local sample activity - no game process is running.", 386, 471, 15, subdued);
        break;
    case 1:
        canvas.draw_text("Activities", 386, 332, 22, foreground);
        canvas.draw_text("No active game or activity service is connected.", 386, 383, 16, subdued);
        canvas.draw_text("Sample activity: Explore the local interface preview.", 386, 426, 15, subdued);
        break;
    case 2:
        canvas.draw_text("Trophies", 386, 332, 22, foreground);
        canvas.draw_text("Local test profile: no trophies earned.", 386, 383, 16, subdued);
        canvas.draw_text("Trophy synchronization is unavailable offline.", 386, 426, 15, subdued);
        break;
    case 3:
        canvas.draw_text("Related content", 386, 332, 22, foreground);
        canvas.draw_text("No broadcasts or community content is connected.", 386, 383, 16, subdued);
        canvas.draw_text("This page uses local test data only.", 386, 426, 15, subdued);
        break;
    }

    canvas.draw_text(
        status_message_.empty()
            ? "Left / Right: sections    Enter: select    Esc: return to Home"
            : status_message_,
        62, 648, 14, foreground
    );
}

const std::string& GameHubScreen::status_message() const {
    return status_message_;
}

} // namespace nexus::shell
