# UI System - Text Rendering

## Overview
Implement a clean UI API for rendering crisp text on screen with positioning, anchoring, and color support. Text must not be blurry and should preserve positioning on window resize.

## Scope (MVP)
- **Only text rendering** - No buttons, containers, or other elements for now
- Manual positioning with padding support
- Anchor-based positioning (TopLeft, TopRight, BottomLeft, BottomRight, Center)
- Color support for text
- Crisp, non-blurry text rendering
- Preserve positions on window resize

## File Structure
```
src/ui/
  UIElement.hpp          - Base class for all UI elements
  Text.hpp               - Text element class
  Text.cpp               - Text element implementation
  UIManager.hpp           - Main UI manager
  UIManager.cpp           - UI manager implementation
  font/
    Font.hpp             - Font loading (stb_truetype)
    Font.cpp             - Font implementation
    stb_truetype.h       - Header-only library

assets/fonts/
  Roboto-Regular.ttf     - Font file
```

## Implementation Steps

### 1. Core Infrastructure
- Create `src/ui/Anchor.hpp` - Anchor enum
- Create `src/ui/UIElement.hpp` - Base class with position, anchor, padding, color
- Add `Mat4::ortho()` to `src/math/mat4.hpp`
- Add `setVec3()` to `src/graphics/shader.hpp` and `src/graphics/shader.cpp`

### 2. Font System
- Download `stb_truetype.h` to `src/ui/font/`
- Create `src/ui/font/Font.hpp` and `Font.cpp`:
  - Load `.ttf` font using stbtt
  - Generate font atlas texture
  - Provide `measureText()` and `getGlyph()` methods
- Download `Roboto-Regular.ttf` to `assets/fonts/`

### 3. Text Element
- Create `src/ui/Text.hpp` and `Text.cpp`:
  - Inherit from UIElement
  - Render text as textured quads using Font atlas
  - Calculate text size for anchoring
  - Use font atlas UVs for each glyph

### 4. Shaders
- Create UI shaders (in-memory in UIManager):
  - Vertex shader: Orthographic projection + model matrix
  - Fragment shader: Color + alpha from font texture

### 5. UIManager
- Create `src/ui/UIManager.hpp` and `UIManager.cpp`:
  - `init()` - Load font, create shaders
  - `shutdown()` - Clean up
  - `update()` - Handle window size changes for anchoring
  - `draw()` - Render all text elements with blending enabled
  - `text()` - Factory method to create Text elements

### 6. Integration
- Add `UIManager` to `src/app/App.hpp`
- Initialize UI in `App::init()`
- Call `m_ui.update()` and `m_ui.draw()` in `App::run()` after 3D render
- Shutdown UI in `App::shutdown()`
- Update `CMakeLists.txt` to include UI source files

### 7. Testing
- Add test text in `App::init()`:
  ```cpp
  m_ui.text("Hello UI!")
      .position(20, 20)
      .color(1, 1, 1);

  m_ui.text("Top Right")
      .anchor(Anchor::TopRight)
      .padding(20)
      .color(0, 1, 0);
  ```
- Run and verify:
  - Text displays clearly (no blur)
  - Anchoring works correctly
  - Resizing window preserves positions

## Key Technical Details

### Crisp Text
- Use stb_truetype to rasterize glyphs at correct size
- Linear filtering on atlas texture (GL_LINEAR)
- Render as textured quads with proper UVs

### Orthographic Projection
```cpp
Mat4::ortho(0, width, height, 0, -1, 1)
```
Pixel-perfect 2D rendering.

### Anchoring Logic
```cpp
switch (anchor) {
    case TopLeft:     (padding, padding) + pos
    case TopRight:    (width - padding - size.x, padding) + pos
    case BottomLeft:  (padding, height - padding - size.y) + pos
    case BottomRight: (width - padding - size.x, height - padding - size.y) + pos
    case Center:       ((width - size.x) / 2, (height - size.y) / 2) + pos
}
```

### Rendering Order
1. Disable depth test
2. Enable blending (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
3. Draw all UI elements
4. Re-enable depth test
5. Disable blending

## Future Enhancements (Out of Scope)
- Buttons with click detection
- Containers for layout
- Hover/click states
- Auto-layout systems
- Scrolling
- Additional text styles (bold, italic)
