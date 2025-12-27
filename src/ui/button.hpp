#pragma once

#include "ui/ui_element.hpp"
#include <string>
#include <functional>

namespace nuage {

class Font;
class App;

/**
 * @brief A clickable UI button with text and hover states.
 */
class Button : public UIElement {
public:
    using ClickCallback = std::function<void()>;

    Button(const std::string& text, Font* font, App* app);

    void onClick(ClickCallback callback) { m_callback = std::move(callback); }
    
    const Vec3& getSize() const { return m_size; }
    const std::string& getText() const { return m_text; }
    const Vec3& getHoverColor() const { return m_hoverColor; }
    float getCornerRadius() const { return m_cornerRadius; }
    bool isOutlineOnly() const { return m_outlineOnly; }
    const Vec3& getOutlineColor() const { return m_outlineColor; }
    float getOutlineThickness() const { return m_outlineThickness; }

    bool isHovered() const { return m_isHovered; }
    void setHovered(bool hovered) { m_isHovered = hovered; }

    void triggerClick() { if (m_callback) m_callback(); }

    // Chaining Overrides
    Button& size(float w, float h) { m_size = Vec3(w, h, 0); return *this; }
    Button& hoverColor(const Vec3& c) { m_hoverColor = c; return *this; }
    Button& text(const std::string& t) { m_text = t; return *this; }
    Button& cornerRadius(float r) { m_cornerRadius = r; return *this; }
    Button& outlineOnly(bool enabled) { m_outlineOnly = enabled; return *this; }
    Button& outlineColor(const Vec3& c) { m_outlineColor = c; return *this; }
    Button& outlineThickness(float t) { m_outlineThickness = t; return *this; }
    
    // UIElement overrides for chaining
    Button& pos(float x, float y) { UIElement::pos(x, y); return *this; }
    Button& anchorMode(Anchor a) { UIElement::anchorMode(a); return *this; }
    Button& colorR(float r, float g, float b) { UIElement::colorR(r, g, b); return *this; }

    Vec3 getAnchoredPosition(int windowWidth, int windowHeight) const;

private:
    std::string m_text;
    Font* m_font = nullptr;
    App* m_app = nullptr;
    Vec3 m_size = {200, 50, 0};
    Vec3 m_hoverColor = {0.8f, 0.8f, 0.8f};
    float m_cornerRadius = 8.0f;
    bool m_outlineOnly = false;
    Vec3 m_outlineColor = {1.0f, 1.0f, 1.0f};
    float m_outlineThickness = 2.0f;
    bool m_isHovered = false;
    ClickCallback m_callback;
};

} // namespace nuage
