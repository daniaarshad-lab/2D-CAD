// ============================================================
//   Shape Master CAD - 2D CAD System using SFML
//   Semester Project | C++ with SFML Graphics
//
//   COMPILE COMMAND (Windows MinGW):
//   g++ main.cpp -o CAD.exe -IC:/SFML/include -LC:/SFML/lib
//       -lsfml-graphics -lsfml-window -lsfml-system
//
//   COMPILE COMMAND (Linux):
//   g++ main.cpp -o CAD -lsfml-graphics -lsfml-window -lsfml-system
//
//   CONTROLS:
//   [1] Line tool      [2] Circle tool    [3] Rectangle tool
//   [4] Polygon tool   [C] Clear canvas   [Z] Undo last shape
//   [R] Red color      [G] Green color    [B] Blue color
//   [K] Black color    [W] White/erase    [+/-] Thickness
//   Left Click  = Start/add point        Right Click = Finish polygon
// ============================================================

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <cmath>
#include <sstream>
#include <iomanip>

// ─────────────────────────────────────────────
//  Utility: distance between two points
// ─────────────────────────────────────────────
float dist(sf::Vector2f a, sf::Vector2f b) {
    float dx = b.x - a.x, dy = b.y - a.y;
    return std::sqrt(dx * dx + dy * dy);
}

// ─────────────────────────────────────────────
//  Shape types
// ─────────────────────────────────────────────
enum class ShapeType { LINE, CIRCLE, RECTANGLE, POLYGON };

// ─────────────────────────────────────────────
//  A stored shape (after finishing drawing)
// ─────────────────────────────────────────────
struct DrawnShape {
    ShapeType type;
    std::vector<sf::Vector2f> points; // start+end for line/rect, center+edge for circle, all verts for polygon
    sf::Color color;
    float thickness;

    // Geometric properties
    float getLength() const {
        if (type == ShapeType::LINE && points.size() == 2)
            return dist(points[0], points[1]);
        return 0.f;
    }

    float getRadius() const {
        if (type == ShapeType::CIRCLE && points.size() == 2)
            return dist(points[0], points[1]);
        return 0.f;
    }

    float getArea() const {
        if (type == ShapeType::CIRCLE) {
            float r = getRadius();
            return 3.14159f * r * r;
        }
        if (type == ShapeType::RECTANGLE && points.size() == 2) {
            return std::abs(points[1].x - points[0].x) * std::abs(points[1].y - points[0].y);
        }
        if (type == ShapeType::POLYGON && points.size() >= 3) {
            // Shoelace formula
            float area = 0.f;
            int n = points.size();
            for (int i = 0; i < n; i++) {
                int j = (i + 1) % n;
                area += points[i].x * points[j].y;
                area -= points[j].x * points[i].y;
            }
            return std::abs(area) / 2.f;
        }
        return 0.f;
    }

    float getPerimeter() const {
        if (type == ShapeType::CIRCLE) {
            return 2.f * 3.14159f * getRadius();
        }
        if (type == ShapeType::RECTANGLE && points.size() == 2) {
            float w = std::abs(points[1].x - points[0].x);
            float h = std::abs(points[1].y - points[0].y);
            return 2.f * (w + h);
        }
        if (type == ShapeType::POLYGON && points.size() >= 2) {
            float p = 0.f;
            for (size_t i = 0; i < points.size(); i++)
                p += dist(points[i], points[(i + 1) % points.size()]);
            return p;
        }
        return 0.f;
    }
};

// ─────────────────────────────────────────────
//  Draw a thick line using a rectangle
// ─────────────────────────────────────────────
void drawThickLine(sf::RenderWindow& win, sf::Vector2f a, sf::Vector2f b,
                   sf::Color col, float thickness) {
    float length = dist(a, b);
    if (length < 1.f) return;
    sf::RectangleShape line(sf::Vector2f(length, thickness));
    line.setFillColor(col);
    line.setOrigin(0, thickness / 2.f);
    line.setPosition(a);
    float angle = std::atan2(b.y - a.y, b.x - a.x) * 180.f / 3.14159f;
    line.setRotation(angle);
    win.draw(line);
}

// ─────────────────────────────────────────────
//  Draw a finished DrawnShape
// ─────────────────────────────────────────────
void renderShape(sf::RenderWindow& win, const DrawnShape& s) {
    if (s.type == ShapeType::LINE && s.points.size() == 2) {
        drawThickLine(win, s.points[0], s.points[1], s.color, s.thickness);
    }
    else if (s.type == ShapeType::CIRCLE && s.points.size() == 2) {
        float r = dist(s.points[0], s.points[1]);
        sf::CircleShape c(r);
        c.setOrigin(r, r);
        c.setPosition(s.points[0]);
        c.setFillColor(sf::Color::Transparent);
        c.setOutlineColor(s.color);
        c.setOutlineThickness(s.thickness);
        win.draw(c);
    }
    else if (s.type == ShapeType::RECTANGLE && s.points.size() == 2) {
        float x = std::min(s.points[0].x, s.points[1].x);
        float y = std::min(s.points[0].y, s.points[1].y);
        float w = std::abs(s.points[1].x - s.points[0].x);
        float h = std::abs(s.points[1].y - s.points[0].y);
        sf::RectangleShape rect(sf::Vector2f(w, h));
        rect.setPosition(x, y);
        rect.setFillColor(sf::Color::Transparent);
        rect.setOutlineColor(s.color);
        rect.setOutlineThickness(s.thickness);
        win.draw(rect);
    }
    else if (s.type == ShapeType::POLYGON && s.points.size() >= 2) {
        for (size_t i = 0; i < s.points.size(); i++) {
            drawThickLine(win, s.points[i],
                          s.points[(i + 1) % s.points.size()],
                          s.color, s.thickness);
        }
    }
}

// ─────────────────────────────────────────────
//  Format float nicely
// ─────────────────────────────────────────────
std::string fmt(float v) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1) << v;
    return ss.str();
}

// ─────────────────────────────────────────────
//  MAIN
// ─────────────────────────────────────────────
int main() {
    const int WIN_W = 1100, WIN_H = 700;
    const int SIDEBAR = 240;
    const int CANVAS_W = WIN_W - SIDEBAR;

    sf::RenderWindow window(sf::VideoMode(WIN_W, WIN_H), "Shape Master CAD",
                            sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);

    // ── Font ──
    sf::Font font;
    // Try to load a system font; fall back gracefully if not found
    bool fontLoaded = font.loadFromFile("C:/Windows/Fonts/arial.ttf");
    if (!fontLoaded) fontLoaded = font.loadFromFile("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf");
    if (!fontLoaded) fontLoaded = font.loadFromFile("/usr/share/fonts/TTF/DejaVuSans.ttf");

    // ── State ──
    ShapeType currentTool = ShapeType::LINE;
    sf::Color currentColor = sf::Color::Black;
    float currentThickness = 2.f;
    std::vector<DrawnShape> shapes;
    std::string statusMsg = "Left click to start drawing";

    // Drawing state
    bool isDrawing = false;
    sf::Vector2f startPt;
    std::vector<sf::Vector2f> polyPts; // for polygon

    // Last selected shape info
    std::string infoText = "";

    // ── Helper: tool name ──
    auto toolName = [](ShapeType t) -> std::string {
        switch (t) {
            case ShapeType::LINE:      return "Line [1]";
            case ShapeType::CIRCLE:    return "Circle [2]";
            case ShapeType::RECTANGLE: return "Rect [3]";
            case ShapeType::POLYGON:   return "Polygon [4]";
        }
        return "";
    };

    // ── Helper: update info text from last shape ──
    auto updateInfo = [&]() {
        if (shapes.empty()) { infoText = ""; return; }
        const DrawnShape& s = shapes.back();
        std::ostringstream ss;
        switch (s.type) {
            case ShapeType::LINE:
                ss << "Line\nLength: " << fmt(s.getLength()) << " px";
                break;
            case ShapeType::CIRCLE:
                ss << "Circle\nRadius: " << fmt(s.getRadius()) << " px\n"
                   << "Area: " << fmt(s.getArea()) << " px2\n"
                   << "Circumference: " << fmt(s.getPerimeter()) << " px";
                break;
            case ShapeType::RECTANGLE:
                ss << "Rectangle\nArea: " << fmt(s.getArea()) << " px2\n"
                   << "Perimeter: " << fmt(s.getPerimeter()) << " px";
                break;
            case ShapeType::POLYGON:
                ss << "Polygon (" << s.points.size() << " pts)\n"
                   << "Area: " << fmt(s.getArea()) << " px2\n"
                   << "Perimeter: " << fmt(s.getPerimeter()) << " px";
                break;
        }
        infoText = ss.str();
    };

    // ── Main loop ──
    while (window.isOpen()) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        sf::Vector2f mouse(mousePos.x, mousePos.y);
        // Clamp to canvas
        bool inCanvas = mouse.x < CANVAS_W && mouse.x > 0 && mouse.y > 0 && mouse.y < WIN_H;

        // ── Events ──
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            // Key controls
            if (event.type == sf::Event::KeyPressed) {
                switch (event.key.code) {
                    case sf::Keyboard::Num1: currentTool = ShapeType::LINE;      isDrawing = false; polyPts.clear(); statusMsg = "Line tool"; break;
                    case sf::Keyboard::Num2: currentTool = ShapeType::CIRCLE;    isDrawing = false; polyPts.clear(); statusMsg = "Circle tool"; break;
                    case sf::Keyboard::Num3: currentTool = ShapeType::RECTANGLE; isDrawing = false; polyPts.clear(); statusMsg = "Rectangle tool"; break;
                    case sf::Keyboard::Num4: currentTool = ShapeType::POLYGON;   isDrawing = false; polyPts.clear(); statusMsg = "Polygon tool — right click to close"; break;
                    case sf::Keyboard::R:    currentColor = sf::Color::Red;      statusMsg = "Color: Red"; break;
                    case sf::Keyboard::G:    currentColor = sf::Color(0,160,0);  statusMsg = "Color: Green"; break;
                    case sf::Keyboard::B:    currentColor = sf::Color(30,100,220);statusMsg = "Color: Blue"; break;
                    case sf::Keyboard::K:    currentColor = sf::Color::Black;    statusMsg = "Color: Black"; break;
                    case sf::Keyboard::W:    currentColor = sf::Color::White;    statusMsg = "Color: White (erase)"; break;
                    case sf::Keyboard::C:    shapes.clear(); infoText = "";      statusMsg = "Canvas cleared"; break;
                    case sf::Keyboard::Z:
                        if (!shapes.empty()) { shapes.pop_back(); updateInfo(); statusMsg = "Undo"; }
                        break;
                    case sf::Keyboard::Equal: // + key
                        currentThickness = std::min(currentThickness + 1.f, 20.f);
                        statusMsg = "Thickness: " + fmt(currentThickness);
                        break;
                    case sf::Keyboard::Dash:  // - key
                        currentThickness = std::max(currentThickness - 1.f, 1.f);
                        statusMsg = "Thickness: " + fmt(currentThickness);
                        break;
                    default: break;
                }
            }

            // Mouse click
            if (event.type == sf::Event::MouseButtonPressed && inCanvas) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    if (currentTool == ShapeType::POLYGON) {
                        if (!isDrawing) {
                            polyPts.clear();
                            isDrawing = true;
                        }
                        polyPts.push_back(mouse);
                        statusMsg = "Polygon: " + std::to_string(polyPts.size()) + " pts — right click to finish";
                    } else {
                        startPt = mouse;
                        isDrawing = true;
                        statusMsg = "Drawing...";
                    }
                }
                if (event.mouseButton.button == sf::Mouse::Right) {
                    if (currentTool == ShapeType::POLYGON && polyPts.size() >= 3) {
                        DrawnShape s;
                        s.type = ShapeType::POLYGON;
                        s.points = polyPts;
                        s.color = currentColor;
                        s.thickness = currentThickness;
                        shapes.push_back(s);
                        polyPts.clear();
                        isDrawing = false;
                        updateInfo();
                        statusMsg = "Polygon saved";
                    } else {
                        isDrawing = false;
                        polyPts.clear();
                        statusMsg = "Cancelled";
                    }
                }
            }

            // Mouse release (for line, circle, rect)
            if (event.type == sf::Event::MouseButtonReleased && inCanvas) {
                if (event.mouseButton.button == sf::Mouse::Left &&
                    isDrawing && currentTool != ShapeType::POLYGON) {
                    sf::Vector2f endPt = mouse;
                    if (dist(startPt, endPt) > 3.f) {
                        DrawnShape s;
                        s.type = currentTool;
                        s.points = { startPt, endPt };
                        s.color = currentColor;
                        s.thickness = currentThickness;
                        shapes.push_back(s);
                        updateInfo();
                        statusMsg = "Shape added";
                    }
                    isDrawing = false;
                }
            }
        }

        // ── Render ──
        window.clear(sf::Color(245, 245, 240));

        // Canvas background
        sf::RectangleShape canvas(sf::Vector2f(CANVAS_W, WIN_H));
        canvas.setFillColor(sf::Color::White);
        canvas.setOutlineColor(sf::Color(200, 200, 200));
        canvas.setOutlineThickness(1.f);
        window.draw(canvas);

        // Grid (subtle)
        for (int x = 0; x < CANVAS_W; x += 40) {
            sf::Vertex line[] = {
                sf::Vertex(sf::Vector2f(x, 0), sf::Color(230, 230, 230)),
                sf::Vertex(sf::Vector2f(x, WIN_H), sf::Color(230, 230, 230))
            };
            window.draw(line, 2, sf::Lines);
        }
        for (int y = 0; y < WIN_H; y += 40) {
            sf::Vertex line[] = {
                sf::Vertex(sf::Vector2f(0, y), sf::Color(230, 230, 230)),
                sf::Vertex(sf::Vector2f(CANVAS_W, y), sf::Color(230, 230, 230))
            };
            window.draw(line, 2, sf::Lines);
        }

        // Draw saved shapes
        for (const auto& s : shapes)
            renderShape(window, s);

        // Draw preview (live as mouse moves)
        if (isDrawing && inCanvas) {
            if (currentTool == ShapeType::LINE) {
                drawThickLine(window, startPt, mouse, currentColor, currentThickness);
            }
            else if (currentTool == ShapeType::CIRCLE) {
                float r = dist(startPt, mouse);
                sf::CircleShape c(r);
                c.setOrigin(r, r);
                c.setPosition(startPt);
                c.setFillColor(sf::Color::Transparent);
                c.setOutlineColor(sf::Color(currentColor.r, currentColor.g, currentColor.b, 150));
                c.setOutlineThickness(currentThickness);
                window.draw(c);
            }
            else if (currentTool == ShapeType::RECTANGLE) {
                float x = std::min(startPt.x, mouse.x);
                float y = std::min(startPt.y, mouse.y);
                float w = std::abs(mouse.x - startPt.x);
                float h = std::abs(mouse.y - startPt.y);
                sf::RectangleShape rect(sf::Vector2f(w, h));
                rect.setPosition(x, y);
                rect.setFillColor(sf::Color::Transparent);
                rect.setOutlineColor(sf::Color(currentColor.r, currentColor.g, currentColor.b, 150));
                rect.setOutlineThickness(currentThickness);
                window.draw(rect);
            }
            else if (currentTool == ShapeType::POLYGON && !polyPts.empty()) {
                // Draw lines between existing polygon points
                for (size_t i = 0; i + 1 < polyPts.size(); i++)
                    drawThickLine(window, polyPts[i], polyPts[i + 1], currentColor, currentThickness);
                // Line from last point to mouse
                drawThickLine(window, polyPts.back(), mouse,
                              sf::Color(currentColor.r, currentColor.g, currentColor.b, 150),
                              currentThickness);
                // Dots at vertices
                for (const auto& pt : polyPts) {
                    sf::CircleShape dot(4);
                    dot.setOrigin(4, 4);
                    dot.setPosition(pt);
                    dot.setFillColor(currentColor);
                    window.draw(dot);
                }
            }
        }

        // ── Sidebar ──
        sf::RectangleShape sidebar(sf::Vector2f(SIDEBAR, WIN_H));
        sidebar.setPosition(CANVAS_W, 0);
        sidebar.setFillColor(sf::Color(30, 32, 40));
        window.draw(sidebar);

        // Sidebar separator
        sf::RectangleShape sep(sf::Vector2f(2, WIN_H));
        sep.setPosition(CANVAS_W, 0);
        sep.setFillColor(sf::Color(60, 140, 220));
        window.draw(sep);

        if (fontLoaded) {
            float sx = CANVAS_W + 16;
            float sy = 16;

            // Title
            sf::Text title("SHAPE MASTER", font, 15);
            title.setFillColor(sf::Color(80, 180, 255));
            title.setStyle(sf::Text::Bold);
            title.setPosition(sx, sy);
            window.draw(title);

            sf::Text sub("CAD SYSTEM", font, 10);
            sub.setFillColor(sf::Color(120, 130, 150));
            sub.setPosition(sx, sy + 20);
            window.draw(sub);

            // Divider
            sf::RectangleShape div1(sf::Vector2f(SIDEBAR - 20, 1));
            div1.setPosition(sx, sy + 40);
            div1.setFillColor(sf::Color(60, 65, 80));
            window.draw(div1);

            // Tools
            sf::Text toolsLabel("TOOLS", font, 10);
            toolsLabel.setFillColor(sf::Color(100, 110, 130));
            toolsLabel.setPosition(sx, sy + 50);
            window.draw(toolsLabel);

            std::vector<std::pair<std::string, ShapeType>> tools = {
                {"[1] Line",      ShapeType::LINE},
                {"[2] Circle",    ShapeType::CIRCLE},
                {"[3] Rectangle", ShapeType::RECTANGLE},
                {"[4] Polygon",   ShapeType::POLYGON},
            };
            for (int i = 0; i < (int)tools.size(); i++) {
                bool active = currentTool == tools[i].second;
                if (active) {
                    sf::RectangleShape hi(sf::Vector2f(SIDEBAR - 20, 22));
                    hi.setPosition(sx - 4, sy + 66 + i * 26);
                    hi.setFillColor(sf::Color(50, 120, 200, 80));
                    window.draw(hi);
                }
                sf::Text t(tools[i].first, font, 12);
                t.setFillColor(active ? sf::Color(80, 180, 255) : sf::Color(180, 185, 200));
                t.setPosition(sx, sy + 68 + i * 26);
                window.draw(t);
            }

            float y2 = sy + 185;
            sf::RectangleShape div2(sf::Vector2f(SIDEBAR - 20, 1));
            div2.setPosition(sx, y2);
            div2.setFillColor(sf::Color(60, 65, 80));
            window.draw(div2);

            sf::Text colLabel("COLOR  [R] [G] [B] [K] [W]", font, 10);
            colLabel.setFillColor(sf::Color(100, 110, 130));
            colLabel.setPosition(sx, y2 + 8);
            window.draw(colLabel);

            // Color swatch
            sf::RectangleShape swatch(sf::Vector2f(28, 18));
            swatch.setPosition(sx, y2 + 24);
            swatch.setFillColor(currentColor);
            swatch.setOutlineColor(sf::Color(100, 110, 130));
            swatch.setOutlineThickness(1);
            window.draw(swatch);

            sf::Text colorName("Active color", font, 10);
            colorName.setFillColor(sf::Color(140, 150, 170));
            colorName.setPosition(sx + 34, y2 + 28);
            window.draw(colorName);

            float y3 = y2 + 58;
            sf::RectangleShape div3(sf::Vector2f(SIDEBAR - 20, 1));
            div3.setPosition(sx, y3);
            div3.setFillColor(sf::Color(60, 65, 80));
            window.draw(div3);

            sf::Text thickLabel("THICKNESS  [+] [-]", font, 10);
            thickLabel.setFillColor(sf::Color(100, 110, 130));
            thickLabel.setPosition(sx, y3 + 8);
            window.draw(thickLabel);

            sf::Text thickVal(fmt(currentThickness) + " px", font, 14);
            thickVal.setFillColor(sf::Color(80, 180, 255));
            thickVal.setPosition(sx, y3 + 24);
            window.draw(thickVal);

            // Thickness bar
            sf::RectangleShape thickBg(sf::Vector2f(SIDEBAR - 26, 6));
            thickBg.setPosition(sx, y3 + 46);
            thickBg.setFillColor(sf::Color(55, 60, 75));
            window.draw(thickBg);
            sf::RectangleShape thickBar(sf::Vector2f((currentThickness / 20.f) * (SIDEBAR - 26), 6));
            thickBar.setPosition(sx, y3 + 46);
            thickBar.setFillColor(sf::Color(60, 140, 220));
            window.draw(thickBar);

            float y4 = y3 + 70;
            sf::RectangleShape div4(sf::Vector2f(SIDEBAR - 20, 1));
            div4.setPosition(sx, y4);
            div4.setFillColor(sf::Color(60, 65, 80));
            window.draw(div4);

            sf::Text actionsLabel("ACTIONS", font, 10);
            actionsLabel.setFillColor(sf::Color(100, 110, 130));
            actionsLabel.setPosition(sx, y4 + 8);
            window.draw(actionsLabel);

            sf::Text actZ("[Z] Undo last shape", font, 11);
            actZ.setFillColor(sf::Color(180, 185, 200));
            actZ.setPosition(sx, y4 + 24);
            window.draw(actZ);

            sf::Text actC("[C] Clear canvas", font, 11);
            actC.setFillColor(sf::Color(180, 185, 200));
            actC.setPosition(sx, y4 + 40);
            window.draw(actC);

            // Shape count
            sf::Text countLabel(std::to_string(shapes.size()) + " shape(s) on canvas", font, 10);
            countLabel.setFillColor(sf::Color(100, 110, 130));
            countLabel.setPosition(sx, y4 + 60);
            window.draw(countLabel);

            // ── Info panel (geometric properties) ──
            float y5 = y4 + 88;
            sf::RectangleShape div5(sf::Vector2f(SIDEBAR - 20, 1));
            div5.setPosition(sx, y5);
            div5.setFillColor(sf::Color(60, 65, 80));
            window.draw(div5);

            sf::Text infoLabel("LAST SHAPE INFO", font, 10);
            infoLabel.setFillColor(sf::Color(100, 110, 130));
            infoLabel.setPosition(sx, y5 + 8);
            window.draw(infoLabel);

            if (!infoText.empty()) {
                // Split by \n and draw each line
                std::istringstream iss(infoText);
                std::string line;
                float iy = y5 + 24;
                bool first = true;
                while (std::getline(iss, line)) {
                    sf::Text lt(line, font, first ? 13 : 11);
                    lt.setFillColor(first ? sf::Color(80, 180, 255) : sf::Color(200, 205, 220));
                    lt.setPosition(sx, iy);
                    window.draw(lt);
                    iy += first ? 18 : 16;
                    first = false;
                }
            } else {
                sf::Text noInfo("Draw a shape to see", font, 10);
                noInfo.setFillColor(sf::Color(70, 80, 100));
                noInfo.setPosition(sx, y5 + 24);
                window.draw(noInfo);
                sf::Text noInfo2("its properties here", font, 10);
                noInfo2.setFillColor(sf::Color(70, 80, 100));
                noInfo2.setPosition(sx, y5 + 38);
                window.draw(noInfo2);
            }

            // ── Status bar ──
            sf::RectangleShape statusBg(sf::Vector2f(CANVAS_W, 28));
            statusBg.setPosition(0, WIN_H - 28);
            statusBg.setFillColor(sf::Color(30, 32, 40));
            window.draw(statusBg);

            sf::Text toolIndicator("Tool: " + toolName(currentTool) + "   |   ", font, 11);
            toolIndicator.setFillColor(sf::Color(80, 180, 255));
            toolIndicator.setPosition(10, WIN_H - 20);
            window.draw(toolIndicator);

            sf::Text statusText(statusMsg, font, 11);
            statusText.setFillColor(sf::Color(180, 185, 200));
            statusText.setPosition(180, WIN_H - 20);
            window.draw(statusText);

            // Mouse coords
            std::ostringstream coordss;
            coordss << "X:" << (int)mouse.x << " Y:" << (int)mouse.y;
            sf::Text coords(coordss.str(), font, 11);
            coords.setFillColor(sf::Color(100, 110, 130));
            coords.setPosition(CANVAS_W - 120, WIN_H - 20);
            window.draw(coords);
        }

        window.display();
    }

    return 0;
}