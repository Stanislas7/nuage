#pragma once

#include "ui/ui_element.hpp"
#include <string>

namespace nuage {

class Font;
struct Vec3;

class Text : public UIElement {
public:
    Text(const std::string& content, Font* font);
    ~Text() = default;

    Text& content(const std::string& text);
    const std::string& getContent() const { return m_content; }

    Vec3 getAnchoredPosition(int windowWidth, int windowHeight) const;
    Vec3 getSize() const;

private:
    std::string m_content;
    Font* m_font = nullptr;
};

}
