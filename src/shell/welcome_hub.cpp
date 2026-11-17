#include "nexus/shell/welcome_hub.hpp"

#include "nexus/core/file_utils.hpp"
#include "nexus/ui/theme.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <fstream>
#include <system_error>
#include <utility>

namespace nexus::shell {
namespace {

using ui::Color;
using ui::Component;
using ui::Rect;

constexpr std::array<Color, 3> background_colors{
    Color{26, 78, 151, 255},
    Color{36, 112, 145, 255},
    Color{80, 65, 151, 255},
};
constexpr std::array<int, 3> column_x{54, 408, 754};
constexpr std::array<int, 3> column_width{336, 328, 470};

bool parse_integer(std::string_view text, int& value) {
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    return error == std::errc{} && end == text.data() + text.size();
}

bool parse_boolean(std::string_view text, bool& value) {
    if (text == "0") {
        value = false;
        return true;
    }
    if (text == "1") {
        value = true;
        return true;
    }
    return false;
}

Component text_component(std::string id, std::string label, Rect bounds, int size, Color color) {
    return {
        std::move(id),
        std::move(label),
        bounds,
        {0, 0, 0, 0},
        color,
        size,
        false,
        false,
        false,
    };
}

} // namespace

WelcomeHub::WelcomeHub(std::filesystem::path layout_path)
    : layout_path_(std::move(layout_path)),
      widgets_{
          {"friends", "Friends Online", "Local sample: 3 profiles offline", 0, 0, true, true},
          {"store", "PlayStation Store", "Unavailable - no online services", 0, 1, true, false},
          {"accessibility", "Accessibility", "Host settings only; other services offline", 1, 0, true, false},
          {"trophies", "Trophies", "Local test profile - no trophies earned", 1, 1, true, true},
          {"battery", "Battery", "No controller battery data available", 2, 0, true, false},
          {"wishlist", "Wishlist", "Local sample only - store is unavailable", 2, 1, true, true},
      },
      backgrounds_{"nexus-night", "nexus-current", "nexus-aurora"} {
    rebuild_scene();
}

bool WelcomeHub::load(std::string& error) {
    error.clear();
    if (layout_path_.empty()) {
        return true;
    }
    std::error_code file_error;
    const bool exists = std::filesystem::exists(layout_path_, file_error);
    if (file_error) {
        error = "Could not inspect Welcome Hub layout: " + file_error.message();
        return false;
    }
    if (!exists) {
        return true;
    }

    std::ifstream input(layout_path_);
    if (!input) {
        error = "Could not open Welcome Hub layout: " + layout_path_.string();
        return false;
    }

    std::vector<WelcomeWidget> loaded_widgets = widgets_;
    std::size_t loaded_background_index = 0;
    std::vector<bool> seen_widgets(loaded_widgets.size(), false);
    bool seen_background = false;
    std::string line;
    std::size_t line_number = 0;
    while (std::getline(input, line)) {
        ++line_number;
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty() || line.front() == '#') {
            continue;
        }
        const std::size_t separator = line.find('=');
        if (separator == std::string::npos) {
            error = "Invalid Welcome Hub layout entry on line " + std::to_string(line_number) + ".";
            return false;
        }
        const std::string key = line.substr(0, separator);
        const std::string value = line.substr(separator + 1);
        if (key == "background") {
            if (seen_background) {
                error = "Duplicate Welcome Hub background on line " + std::to_string(line_number) + ".";
                return false;
            }
            const auto background = std::find(backgrounds_.begin(), backgrounds_.end(), value);
            if (background == backgrounds_.end()) {
                error = "Unknown Welcome Hub background on line " + std::to_string(line_number) + ".";
                return false;
            }
            loaded_background_index = static_cast<std::size_t>(background - backgrounds_.begin());
            seen_background = true;
            continue;
        }

        if (!key.starts_with("widget.")) {
            error = "Unknown Welcome Hub layout key on line " + std::to_string(line_number) + ".";
            return false;
        }
        const std::string widget_id = key.substr(7);
        const auto widget = std::find_if(
            loaded_widgets.begin(),
            loaded_widgets.end(),
            [&widget_id](const WelcomeWidget& candidate) { return candidate.id == widget_id; }
        );
        if (widget == loaded_widgets.end()) {
            error = "Unknown Welcome Hub widget on line " + std::to_string(line_number) + ".";
            return false;
        }
        const std::size_t widget_index = static_cast<std::size_t>(widget - loaded_widgets.begin());
        if (seen_widgets[widget_index]) {
            error = "Duplicate Welcome Hub widget on line " + std::to_string(line_number) + ".";
            return false;
        }
        const std::size_t first = value.find(',');
        const std::size_t second = first == std::string::npos ? first : value.find(',', first + 1);
        const std::size_t third = second == std::string::npos ? second : value.find(',', second + 1);
        if (first == std::string::npos || second == std::string::npos ||
            third == std::string::npos || value.find(',', third + 1) != std::string::npos) {
            error = "Invalid Welcome Hub widget layout on line " + std::to_string(line_number) + ".";
            return false;
        }
        int column = 0;
        int row = 0;
        bool enabled = false;
        bool large = false;
        if (!parse_integer(std::string_view(value).substr(0, first), column) ||
            !parse_integer(std::string_view(value).substr(first + 1, second - first - 1), row) ||
            !parse_boolean(std::string_view(value).substr(second + 1, third - second - 1), enabled) ||
            !parse_boolean(std::string_view(value).substr(third + 1), large) ||
            column < 0 || column >= static_cast<int>(column_x.size()) ||
            row < 0 || row > 1) {
            error = "Invalid Welcome Hub widget layout on line " + std::to_string(line_number) + ".";
            return false;
        }
        widget->column = column;
        widget->row = row;
        widget->enabled = enabled;
        widget->large = large;
        seen_widgets[widget_index] = true;
    }
    if (input.bad()) {
        error = "Error while reading Welcome Hub layout: " + layout_path_.string();
        return false;
    }
    std::array<std::array<bool, 2>, 3> occupied{};
    for (const auto& widget : loaded_widgets) {
        if (occupied[static_cast<std::size_t>(widget.column)][static_cast<std::size_t>(widget.row)]) {
            error = "Welcome Hub layout places multiple widgets in the same grid cell.";
            return false;
        }
        occupied[static_cast<std::size_t>(widget.column)][static_cast<std::size_t>(widget.row)] = true;
    }
    widgets_ = std::move(loaded_widgets);
    background_index_ = loaded_background_index;
    rebuild_scene();
    return true;
}

bool WelcomeHub::handle(input::Action action) {
    using input::Action;
    if (action == Action::options) {
        editing_ = !editing_;
        moving_widget_ = false;
        set_status(editing_
            ? "Widget editor: arrows select; Square/M moves; X toggles; L1/R1 resizes."
            : "Welcome Hub layout preview.");
        rebuild_scene();
        return true;
    }
    if (!editing_) {
        return false;
    }

    switch (action) {
    case Action::move_left:
        return move_widget_or_focus(ui::Direction::left);
    case Action::move_right:
        return move_widget_or_focus(ui::Direction::right);
    case Action::move_up:
        return move_widget_or_focus(ui::Direction::up);
    case Action::move_down:
        return move_widget_or_focus(ui::Direction::down);
    case Action::accept:
    case Action::toggle_widget: {
        if (moving_widget_) {
            moving_widget_ = false;
            set_status("Widget position set.");
            return save_layout();
        }
        auto& widget = widgets_[selected_widget_index()];
        widget.enabled = !widget.enabled;
        set_status(widget.title + (widget.enabled ? " shown." : " hidden."));
        rebuild_scene();
        return save_layout();
    }
    case Action::resize_smaller:
    case Action::resize_larger: {
        auto& widget = widgets_[selected_widget_index()];
        widget.large = action == Action::resize_larger;
        set_status(widget.title + (widget.large ? " resized larger." : " resized smaller."));
        rebuild_scene();
        return save_layout();
    }
    case Action::cycle_background:
        background_index_ = (background_index_ + 1) % backgrounds_.size();
        set_status("Background: " + backgrounds_[background_index_] + ".");
        rebuild_scene();
        return save_layout();
    case Action::place_widget:
        moving_widget_ = !moving_widget_;
        set_status(moving_widget_
            ? "Move mode: use arrows, then press X to place the widget."
            : "Widget position set.");
        if (!moving_widget_) {
            return save_layout();
        }
        return true;
    case Action::back:
        editing_ = false;
        set_status("Welcome Hub layout preview.");
        rebuild_scene();
        return true;
    case Action::ps_button:
    case Action::quit:
    case Action::options:
        return false;
    }
    return false;
}

void WelcomeHub::mouse_move(int x, int y) {
    if (!editing_ || !scene_.focus_at(x, y)) {
        return;
    }
    const std::string_view focused = scene_.focused_id();
    if (focused.starts_with("widget.")) {
        selected_widget_id_ = std::string(focused.substr(7));
    }
}

bool WelcomeHub::mouse_click(int x, int y) {
    mouse_move(x, y);
    if (!editing_ || !scene_.focus_at(x, y)) {
        return false;
    }
    return handle(input::Action::accept);
}

void WelcomeHub::render(ui::Canvas& canvas) const {
    const Color text = high_contrast_ ? ui::theme::high_contrast_text : ui::theme::text;
    canvas.draw_text("Welcome Hub", 42, 170, 23, text);
    canvas.draw_text(
        editing_ ? "LOCAL WIDGET LAYOUT  |  EDIT MODE" : "LOCAL WIDGET LAYOUT  |  OFFLINE",
        776, 174, 13, ui::theme::subdued_text
    );
    scene_.render(canvas);
    canvas.draw_text(
        status_message_.empty()
            ? "Options: edit    X: show / hide    Square / M: move    L1 / R1 or [ ]: resize    B / Y: background"
            : status_message_,
        54, 658, 14, text
    );
}

void WelcomeHub::set_high_contrast(bool enabled) {
    if (high_contrast_ != enabled) {
        high_contrast_ = enabled;
        rebuild_scene();
    }
}

bool WelcomeHub::editing() const {
    return editing_;
}

const std::string& WelcomeHub::status_message() const {
    return status_message_;
}

const std::vector<WelcomeWidget>& WelcomeHub::widgets() const {
    return widgets_;
}

std::string_view WelcomeHub::background_id() const {
    return backgrounds_[background_index_];
}

Color WelcomeHub::background_color() const {
    return background_colors[background_index_];
}

void WelcomeHub::rebuild_scene() {
    std::vector<Component> components;
    components.reserve(widgets_.size() + 1);
    for (std::size_t column = 0; column < column_x.size(); ++column) {
        int y = 314;
        std::vector<const WelcomeWidget*> column_widgets;
        for (const auto& widget : widgets_) {
            if (widget.column == static_cast<int>(column) && (widget.enabled || editing_)) {
                column_widgets.push_back(&widget);
            }
        }
        std::sort(column_widgets.begin(), column_widgets.end(), [](const auto* left, const auto* right) {
            return left->row < right->row;
        });
        for (const auto* widget : column_widgets) {
            const int height = widget->large ? 205 : 102;
            components.push_back({
                "widget." + widget->id,
                widget->title,
                {column_x[column], y, column_width[column], height},
                high_contrast_
                    ? ui::theme::high_contrast_background
                    : (editing_ && !widget->enabled
                        ? Color{34, 38, 48, 235}
                        : Color{12, 19, 32, 235}),
                high_contrast_
                    ? ui::theme::high_contrast_text
                    : (widget->enabled ? ui::theme::text : ui::theme::subdued_text),
                17,
                editing_,
                true,
                editing_ && widget->id == selected_widget_id_,
            });
            components.push_back(text_component(
                "detail." + widget->id,
                widget->detail,
                {column_x[column] + 22, y + 48, column_width[column] - 40, 35},
                14,
                high_contrast_ ? ui::theme::high_contrast_text : ui::theme::subdued_text
            ));
            y += height + 13;
        }
    }
    scene_.set_components(std::move(components));
    if (editing_) {
        static_cast<void>(scene_.focus("widget." + selected_widget_id_));
    }
}

void WelcomeHub::set_status(std::string message) {
    status_message_ = std::move(message);
}

std::size_t WelcomeHub::selected_widget_index() const {
    const auto selected = std::find_if(
        widgets_.begin(),
        widgets_.end(),
        [this](const WelcomeWidget& widget) { return widget.id == selected_widget_id_; }
    );
    return static_cast<std::size_t>(selected - widgets_.begin());
}

bool WelcomeHub::save_layout() {
    if (layout_path_.empty()) {
        set_status(status_message_ + " Layout is not persisted (no path configured).");
        return true;
    }
    std::error_code filesystem_error;
    if (!layout_path_.parent_path().empty()) {
        std::filesystem::create_directories(layout_path_.parent_path(), filesystem_error);
        if (filesystem_error) {
            set_status("Could not save Welcome Hub layout: " + filesystem_error.message());
            return false;
        }
    }
    const std::filesystem::path temporary_path = layout_path_.string() + ".tmp";
    {
        std::ofstream output(temporary_path, std::ios::trunc);
        if (!output) {
            set_status("Could not create Welcome Hub layout: " + temporary_path.string());
            return false;
        }
        output << "background=" << backgrounds_[background_index_] << '\n';
        for (const auto& widget : widgets_) {
            output << "widget." << widget.id << '='
                   << widget.column << ',' << widget.row << ','
                   << (widget.enabled ? 1 : 0) << ',' << (widget.large ? 1 : 0) << '\n';
        }
        output.flush();
        if (!output) {
            set_status("Could not write Welcome Hub layout: " + temporary_path.string());
            output.close();
            std::filesystem::remove(temporary_path, filesystem_error);
            return false;
        }
    }
    std::string replace_error;
    if (!core::replace_file(temporary_path, layout_path_, replace_error)) {
        set_status("Could not replace Welcome Hub layout: " + replace_error);
        std::error_code cleanup_error;
        std::filesystem::remove(temporary_path, cleanup_error);
        return false;
    }
    return true;
}

bool WelcomeHub::move_selected_widget(ui::Direction direction) {
    auto& selected = widgets_[selected_widget_index()];
    int column = selected.column;
    int row = selected.row;
    switch (direction) {
    case ui::Direction::left: --column; break;
    case ui::Direction::right: ++column; break;
    case ui::Direction::up: --row; break;
    case ui::Direction::down: ++row; break;
    }
    if (column < 0 || column >= static_cast<int>(column_x.size()) || row < 0 || row > 1) {
        return false;
    }
    const auto occupied = std::find_if(
        widgets_.begin(),
        widgets_.end(),
        [&selected, column, row](const WelcomeWidget& widget) {
            return widget.id != selected.id && widget.column == column && widget.row == row;
        }
    );
    if (occupied != widgets_.end()) {
        occupied->column = selected.column;
        occupied->row = selected.row;
    }
    selected.column = column;
    selected.row = row;
    set_status("Moved " + selected.title + ".");
    rebuild_scene();
    return save_layout();
}

bool WelcomeHub::move_widget_or_focus(ui::Direction direction) {
    if (moving_widget_) {
        return move_selected_widget(direction);
    }
    if (!scene_.move(direction)) {
        return false;
    }
    const std::string_view focused = scene_.focused_id();
    if (!focused.starts_with("widget.")) {
        return false;
    }
    selected_widget_id_ = std::string(focused.substr(7));
    set_status("Selected " + widgets_[selected_widget_index()].title + ".");
    return true;
}

} // namespace nexus::shell
