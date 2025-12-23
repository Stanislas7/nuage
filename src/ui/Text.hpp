#pragma once

#include "ui/UIElement.hpp"
#include <string>

namespace nuage {

class Font;
class App;
struct Vec3;

class Text : public UIElement {
public:
    Text(const std::string& content, Font* font, App* app);
    ~Text() = default;

    Text& content(const std::string& text);
    const std::string& getContent() const { return m_content; }

    Vec3 getAnchoredPosition(int windowWidth, int windowHeight) const;
    Vec3 getSize() const;

private:
    std::string m_content;
    Font* m_font = nullptr;
    App* m_app = nullptr;
};

}
