#pragma once

#include "nexus/ui/canvas.hpp"

#include <algorithm>

namespace nexus::ui {

class ScalarAnimation {
public:
    explicit ScalarAnimation(float initial = 0.0F) : value_(initial), target_(initial) {}

    void set_target(float target, float duration_seconds) {
        start_ = value_;
        target_ = target;
        duration_ = std::max(duration_seconds, 0.0F);
        elapsed_ = 0.0F;
        active_ = duration_ > 0.0F;
        if (!active_) {
            value_ = target_;
        }
    }

    void update(float delta_seconds) {
        if (!active_ || delta_seconds <= 0.0F) {
            return;
        }
        elapsed_ = std::min(elapsed_ + delta_seconds, duration_);
        const float linear = elapsed_ / duration_;
        const float eased = 1.0F - (1.0F - linear) * (1.0F - linear) * (1.0F - linear);
        value_ = start_ + (target_ - start_) * eased;
        if (elapsed_ >= duration_) {
            value_ = target_;
            active_ = false;
        }
    }

    [[nodiscard]] float value() const {
        return value_;
    }

    [[nodiscard]] bool active() const {
        return active_;
    }

private:
    float value_ = 0.0F;
    float start_ = 0.0F;
    float target_ = 0.0F;
    float duration_ = 0.0F;
    float elapsed_ = 0.0F;
    bool active_ = false;
};

class ColorAnimation {
public:
    explicit ColorAnimation(Color initial = {0, 0, 0, 255}) : value_(initial), target_(initial) {}

    void set_target(Color target, float duration_seconds) {
        start_ = value_;
        target_ = target;
        progress_ = ScalarAnimation(0.0F);
        progress_.set_target(1.0F, duration_seconds);
        if (duration_seconds <= 0.0F) {
            value_ = target_;
        }
    }

    void update(float delta_seconds) {
        progress_.update(delta_seconds);
        const float amount = progress_.value();
        value_ = {
            interpolate(start_.red, target_.red, amount),
            interpolate(start_.green, target_.green, amount),
            interpolate(start_.blue, target_.blue, amount),
            interpolate(start_.alpha, target_.alpha, amount),
        };
        if (!progress_.active()) {
            value_ = target_;
        }
    }

    [[nodiscard]] Color value() const {
        return value_;
    }

private:
    static std::uint8_t interpolate(std::uint8_t from, std::uint8_t to, float amount) {
        const float value = static_cast<float>(from) +
            (static_cast<float>(to) - static_cast<float>(from)) * amount;
        return static_cast<std::uint8_t>(value);
    }

    Color value_;
    Color start_{0, 0, 0, 255};
    Color target_;
    ScalarAnimation progress_{1.0F};
};

} // namespace nexus::ui
