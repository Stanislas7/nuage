#include "ui/button.hpp"
#include "ui/font/font.hpp"

namespace nuage {

Button::Button(const std::string& text, Font* font)
    : m_text(text), m_font(font) {
    color = Vec3(0.5f, 0.5f, 0.5f); // Default background color
}

Vec3 Button::getAnchoredPosition(int windowWidth, int windowHeight) const {
    float x = position.x;
    float y = position.y;

    switch (anchor) {
        case Anchor::TopLeft:     x = padding + position.x; y = padding + position.y; break;
        case Anchor::TopRight:    x = windowWidth - padding - position.x - m_size.x;
                                  y = padding + position.y; break;
        case Anchor::BottomLeft:  x = padding + position.x;
                                  y = windowHeight - padding - position.y - m_size.y; break;
        case Anchor::BottomRight: x = windowWidth - padding - position.x - m_size.x;
                                  y = windowHeight - padding - position.y - m_size.y; break;
        case Anchor::Center:      x = windowWidth / 2.0f + position.x - m_size.x / 2.0f;
                                  y = windowHeight / 2.0f + position.y - m_size.y / 2.0f; break;
    }

    return Vec3(x, y, 0);
}

} // namespace nuage
