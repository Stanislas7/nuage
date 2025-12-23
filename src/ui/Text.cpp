#include "ui/Text.hpp"
#include "ui/font/Font.hpp"
#include "app/App.hpp"
#include <GLFW/glfw3.h>

namespace nuage {

Text::Text(const std::string& content, Font* font, App* app)
    : m_content(content), m_font(font), m_app(app) {
}

Text& Text::content(const std::string& text) {
    m_content = text;
    return *this;
}

Vec3 Text::getAnchoredPosition(int windowWidth, int windowHeight) const {
    Vec3 size = getSize();
    Vec3 result = position;

    switch (anchor) {
        case Anchor::TopLeft:
            result.x = padding + position.x;
            result.y = padding + position.y;
            break;
        case Anchor::TopRight:
            result.x = windowWidth - padding - size.x + position.x;
            result.y = padding + position.y;
            break;
        case Anchor::BottomLeft:
            result.x = padding + position.x;
            result.y = windowHeight - padding - size.y + position.y;
            break;
        case Anchor::BottomRight:
            result.x = windowWidth - padding - size.x + position.x;
            result.y = windowHeight - padding - size.y + position.y;
            break;
        case Anchor::Center:
            result.x = (windowWidth - size.x) / 2.0f + position.x;
            result.y = (windowHeight - size.y) / 2.0f + position.y;
            break;
    }

    return result;
}

Vec3 Text::getSize() const {
    if (m_font) {
        return m_font->measureText(m_content) * scale;
    }
    return Vec3(0, 0, 0);
}

}
