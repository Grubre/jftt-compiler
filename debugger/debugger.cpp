#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <vector>

#include "debugger-wm.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_options.hpp"
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

static constexpr auto value_text_color = Color::GrayDark;
static constexpr auto updated_text_color = Color::Green;

static constexpr auto title_text_color = Color::White;
static constexpr auto title_text_color_focused = Color::Green;

auto number_len(int number) -> int {
    auto len = 0;

    while (number > 0) {
        number /= 10;
        len++;
    }

    return len;
}

class LinesDisplay : public ComponentBase {
  public:
    LinesDisplay(std::vector<std::string> instruction)
        : lines(std::move(instruction)) {}

    Element Render() final {
        Elements elements;

        const auto max_line_number_len = number_len(lines.size());

        auto line_number = 0;
        for (const auto &line : lines) {
            const auto breakpoint = breakpoints.contains(line_number)
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

    void breakpoint(int line) {
        if (breakpoints.find(line) != breakpoints.end()) {
            breakpoints.erase(line);
        } else {
            breakpoints[line] = true;
        }
    }

    void select_line(int line) { selected_line = line; }
    void update_scroll(int scroll) { scrolled = scroll; }

    virtual bool OnEvent(Event event) final {
        if (event.is_mouse()) {
            if (event.mouse().motion == Mouse::Released) {
                breakpoint(event.mouse().y + scrolled);
                return true;
            }
        }
        return false;
    }

  private:
    Box breakpoint_box;
    std::vector<std::string> lines;
    int selected_line = 0;
    int scrolled = 0;
    std::unordered_map<int, bool> breakpoints{};
};

class RegisterDisplay : public ComponentBase {
  public:
    RegisterDisplay(const std::array<long long, 8> &registers, long long lr) {
        update_registers(registers, lr);
        for (auto i = 0u; i < 8u; i++)
            registers_changed[i] = false;
        lr_changed = false;
    }

    Element Render() final {
        Elements elements;

        elements.push_back(text(L"Registers") | bold | size(WIDTH, EQUAL, 13));

        elements.push_back(separator());

        char reg_name = 'a';
        for (auto i = 0u; i < 8u; i++) {
            const auto reg_value_wstr = std::to_wstring(registers[i]);
            const auto value_color =
                registers_changed[i] ? updated_text_color : value_text_color;
            const auto reg = hbox({
                text(std::string{reg_name}) | size(WIDTH, EQUAL, 3) |
                    color(Color::Red),
                text(reg_value_wstr) | color(value_color) | bold |
                    size(WIDTH, EQUAL, 10),
            });
            elements.push_back(reg);
            reg_name++;
        }

        const auto lr_value_color =
            lr_changed ? updated_text_color : value_text_color;
        const auto lr_wstr = std::to_wstring(lr);
        const auto lr_element = hbox({
            text(std::string{"lr"}) | size(WIDTH, EQUAL, 3) | color(Color::Red),
            text(lr_wstr) | color(lr_value_color) | bold |
                size(WIDTH, EQUAL, 10),
        });
        elements.push_back(lr_element);

        return vbox(std::move(elements));
    }

    void update_registers(const std::array<long long, 8> &registers,
                          long long lr) {
        for (auto i = 0u; i < 8u; i++) {
            registers_changed[i] = registers[i] != this->registers[i];
            this->registers[i] = registers[i];
        }
        lr_changed = lr != this->lr;
        this->lr = lr;
    }

  private:
    std::array<bool, 8> registers_changed{false};
    std::array<long long, 8> registers;
    bool lr_changed = false;
    long long lr;
};

class ScrollerBase : public ComponentBase {
  public:
    ScrollerBase(Component child, std::function<void(int)> update_child_scroll)
        : update_child_scroll(update_child_scroll) {
        Add(child);
    }

  private:
    Element Render() final {
        const auto focused = Focused() ? focus : ftxui::select;

        const auto focused_str = Focused() ? "focused" : "";

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

    bool OnEvent(Event event) final {
        if (event.is_mouse() && box_.Contain(event.mouse().x, event.mouse().y))
            TakeFocus();

        if (!Focused())
            return false;
        ComponentBase::OnEvent(event);

        constexpr auto ctrlj = '\n';
        constexpr auto ctrlk = '\v';

        constexpr auto coeff = 1;

        int selected_old = scrolled;
        if (event == Event::Character(ctrlk) || event == Event::ArrowUpCtrl ||
            (event.is_mouse() && event.mouse().button == Mouse::WheelUp)) {
            scrolled -= coeff;
        }
        if ((event == Event::Character(ctrlj) ||
             event == Event::ArrowDownCtrl ||
             (event.is_mouse() && event.mouse().button == Mouse::WheelDown))) {
            scrolled += coeff;
        }
        if (false)
            scrolled += (box_.y_max - box_.y_min) / 1;
        if (false)
            scrolled -= (box_.y_max - box_.y_min) / 1;
        if (false)
            scrolled = 0;
        if (false)
            scrolled = size_;

        scrolled = std::max(box_.y_max / 2,
                            std::min(size_ - (box_.y_max / 2) - 2, scrolled));

        update_child_scroll(scrolled - box_.y_max / 2);

        return selected_old != scrolled;
    }

    bool Focusable() const final { return true; }

    std::function<void(int)> update_child_scroll;

    int scrolled = 0;
    int size_ = 0;
    Box box_;
};

Component Scroller(Component child,
                   std::function<void(int)> update_child_scroll) {
    return Make<ScrollerBase>(std::move(child), update_child_scroll);
}

class MemoryDisplay : public ComponentBase {
  public:
    MemoryDisplay(std::map<long long, long long> *pam) : pam(pam) {}
    Element Render() final {
        Elements elements;

        for (auto &[address, value] : *pam) {
            const auto address_wstr = std::to_wstring(address);
            const auto value_wstr = std::to_wstring(value);
            const auto memory_element = hbox({
                text(address_wstr) | size(WIDTH, EQUAL, 10) | color(Color::Red),
                text(value_wstr) | color(value_text_color) |
                    size(WIDTH, EQUAL, 10),
            });
            elements.push_back(memory_element);
        }

        return vbox(std::move(elements));
    }

    bool OnEvent(Event event) final {
        if (event.is_mouse()) {
            if (event.mouse().motion == Mouse::Released) {
                return true;
            }
        }
        return false;
    }

  private:
    std::map<long long, long long> *pam = nullptr;
};

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

    auto vm = VirtualMachine(instructions);

    for (auto i = 0u; i < 100u; i++) {
        vm.pam[i] = i;
    }

    auto screen = ScreenInteractive::Fullscreen();

    // REGISTERS AND MEMORY DISPLAY
    const auto register_display = Make<RegisterDisplay>(vm.r, vm.lr);

    const auto memory_display = Make<MemoryDisplay>(&vm.pam);
    const auto memory_scroller = Scroller(memory_display, [](int) {});

    const auto memory_renderer = Renderer(memory_scroller, [&] {
        const auto title_color = memory_scroller->Focused()
                                     ? title_text_color_focused
                                     : title_text_color;
        return vbox({text(L"Memory") | bold | size(WIDTH, EQUAL, 13) |
                         color(title_color),
                     separator(), memory_scroller->Render()});
    });

    const auto state_ui = Renderer(memory_renderer, [&] {
        return hbox({register_display->Render(), separator(),
                     memory_renderer->Render()});
    });

    // CONSOLE
    std::vector<std::string> console_lines;
    const auto console_history = Renderer([&] {
        Elements elements;
        for (auto &line : console_lines) {
            elements.push_back(text(line) | size(WIDTH, EQUAL, 100));
        }
        return vbox(std::move(elements));
    });

    std::string console_input_str;
    const auto console_input = Input(&console_input_str, L"> ");

    const auto console_container = Container::Vertical({
        console_input,
    });

    auto console_ui = Renderer(console_container, [&] {
        const auto title_color = console_container->Focused()
                                     ? title_text_color_focused
                                     : title_text_color;
        return vbox({
            text(L"Console") | bold | size(WIDTH, EQUAL, 13) |
                color(title_color),
            separator(),
            vbox({
                console_history->Render(),
                separator(),
                console_input->Render(),
            }),
        });
    });

    const auto memory_and_console_container = Container::Vertical({
        state_ui,
        console_container,
    });

    // LINES DISPLAY
    const auto lines_display = Make<LinesDisplay>(*lines);

    const auto update_lines_renderer_scroll = [&](int scrolled) {
        lines_display->update_scroll(scrolled);
    };

    const auto line_scroller =
        Scroller(lines_display, update_lines_renderer_scroll);

    const auto line_scroller_ui = Renderer(line_scroller, [&] {
        const auto title_color = line_scroller->Focused()
                                     ? title_text_color_focused
                                     : title_text_color;
        return vbox({
            text(L"Instructions") | bold | size(WIDTH, EQUAL, 13) |
                color(title_color),
            separator(),
            line_scroller->Render(),
        });
    });

    const auto ui_container = Container::Horizontal({
        line_scroller_ui,
        memory_and_console_container,
    });

    const auto main_renderer = Renderer(ui_container, [&] {
        return hbox(
            {line_scroller_ui->Render(), separator(),
             vbox({state_ui->Render(), separator(), console_ui->Render()})});
    });

    const auto keyboard_event_handler = CatchEvent([&](Event event) {
        if (event == Event::Character('q') or event == Event::Escape) {
            screen.Exit();
            return true;
        }
        return false;
    });

    screen.Loop(main_renderer | keyboard_event_handler);
    return EXIT_SUCCESS;
}
