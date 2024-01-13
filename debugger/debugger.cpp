#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include "debugger-wm.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/mouse.hpp"
#include "ftxui/dom/deprecated.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/dom/node.hpp"
#include "ftxui/dom/requirement.hpp"
#include "ftxui/screen/box.hpp"
#include "ftxui/screen/screen.hpp"
#include "ftxui/screen/string.hpp"
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>

using namespace ftxui;

auto number_len(int number) -> int {
    auto len = 0;

    while (number > 0) {
        number /= 10;
        len++;
    }

    return len;
}

class LinesDisplayer : public ComponentBase {
  public:
    LinesDisplayer(std::vector<std::string> instruction)
        : lines(std::move(instruction)) {}

    Element Render() final {
        Elements elements;

        const auto max_line_number_len = number_len(lines.size());

        auto line_number = 1;
        for (const auto &line : lines) {
            const auto breakpoint = line_number == break_point_line
                                        ? std::wstring{L"●"}
                                        : std::wstring{L" "};
            const auto line_number_str = std::to_wstring(line_number);
            const auto style =
                line_number == selected_line ? inverted : nothing;
            const auto line_with_number = hbox(
                {text(breakpoint) | size(WIDTH, Constraint::EQUAL, 2) |
                     color(Color::Red),
                 text(line_number_str) | color(Color::GrayDark) | align_right |
                     bold | size(WIDTH, Constraint::EQUAL, max_line_number_len),
                 separator(), text(line) | style});
            elements.push_back(line_with_number);
            line_number++;
        }

        return vbox(std::move(elements));
    }

  private:
    std::vector<std::string> lines;
    int selected_line = 6;
    int break_point_line = 3;
};

class ScrollerBase : public ComponentBase {
  public:
    ScrollerBase(Component child) { Add(child); }

  private:
    Element Render() final {
        const auto focused = Focused() ? focus : ftxui::select;

        Element background = ComponentBase::Render();
        background->ComputeRequirement();
        size_ = background->requirement().min_y;
        return dbox({
                   std::move(background),
                   vbox({
                       text(L"") | size(HEIGHT, EQUAL, scrolled),
                       text(L"") | focused,
                   }),
               }) |
               vscroll_indicator | yframe | yflex | reflect(box_);
    }

    virtual bool OnEvent(Event event) final {
        if (event.is_mouse() && box_.Contain(event.mouse().x, event.mouse().y))
            TakeFocus();

        const auto coeff = 1;

        int selected_old = scrolled;
        if (event == Event::ArrowUp || event == Event::Character('k') ||
            (event.is_mouse() && event.mouse().button == Mouse::WheelUp)) {
            scrolled -= coeff;
        }
        if ((event == Event::ArrowDown || event == Event::Character('j') ||
             (event.is_mouse() && event.mouse().button == Mouse::WheelDown))) {
            scrolled += coeff;
        }
        if (event == Event::ArrowDownCtrl)
            scrolled += (box_.y_max - box_.y_min) / 1;
        if (event == Event::ArrowUpCtrl)
            scrolled -= (box_.y_max - box_.y_min) / 1;
        if (event == Event::Home)
            scrolled = 0;
        if (event == Event::End)
            scrolled = size_;

        scrolled = std::max(box_.y_max / 2,
                            std::min(size_ - (box_.y_max / 2) - 2, scrolled));
        return selected_old != scrolled;
    }

    bool Focusable() const final { return true; }

    int scrolled = 0;
    int size_ = 0;
    Box box_;
};

Component Scroller(Component child) {
    return Make<ScrollerBase>(std::move(child));
}

auto read_files(const std::filesystem::path &filepath)
    -> std::optional<std::vector<std::string>> {
    auto file = std::ifstream{filepath};
    auto lines = std::vector<std::string>{};

    if (!file) {
        return std::nullopt;
    }

    for (std::string line; std::getline(file, line);) {
        lines.push_back(line);
    }

    return lines;
}

void format(std::vector<std::string> &lines) {
    for (auto &line : lines) {
        for (auto i = line.find('\t'); i != std::string::npos;
             i = line.find('\t')) {
            line.replace(i, 1, "    ");
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <file>" << std::endl;
        return EXIT_FAILURE;
    }

    auto lines = read_files(argv[1]);

    if (!lines) {
        std::cout << std::format("Error: File '{}' not found", argv[1])
                  << std::endl;
        return EXIT_FAILURE;
    }

    const auto instructions = parse_lines(*lines);

    format(*lines);

    auto screen = ScreenInteractive::Fullscreen();
    const auto lines_component = Make<LinesDisplayer>(*lines);
    const auto line_renderer = Renderer(lines_component, [&] {
        return hbox({
            lines_component->Render(),
        });
    });

    auto line_scroller = Scroller(line_renderer);

    const auto main_renderer = Renderer(line_scroller, [&] {
        return hbox({
            line_scroller->Render(),
            separator(),
        });
    });

    const auto keyboard_event_handler = CatchEvent([&screen](Event event) {
        if (event == Event::Character('q') or event == Event::Escape) {
            screen.Exit();
            return true;
        }
        return false;
    });

    screen.Loop(main_renderer | keyboard_event_handler);
    return EXIT_SUCCESS;
}
