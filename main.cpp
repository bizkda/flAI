// main.cpp
#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <algorithm>

using namespace ftxui;
using json = nlohmann::json;

// ---------- Model ----------
struct ChatMessage {
    std::string speaker;
    std::string text;
};

struct AppState {
    std::vector<ChatMessage> history;
    std::string inputText;
    int scrollOffset = 0; // 0 = pinned to bottom (latest messages)
};

// ---------- Ollama client ----------
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* out) {
    size_t totalSize = size * nmemb;
    out->append((char*)contents, totalSize);
    return totalSize;
}

std::string callOllama(const std::string& userMessage) {
    CURL* curl = curl_easy_init();
    std::string response;

    if (curl) {
        json requestBody = {
            {"model", "minimax-m3:cloud"},
            {"messages", json::array({
                {{"role", "user"}, {"content", userMessage}}
            })},
            {"stream", false}
        };
        std::string body = requestBody.dump();

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:11434/api/chat");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        curl_easy_perform(curl);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    return response;
}

std::string extractReply(const std::string& rawJson) {
    try {
        json parsed = json::parse(rawJson);
        return parsed["message"]["content"].get<std::string>();
    } catch (...) {
        return "[Failed to parse response]";
    }
}

// ---------- TUI ----------
int main() {
    curl_global_init(CURL_GLOBAL_ALL);

    auto state = std::make_shared<AppState>();
    auto screen = ScreenInteractive::Fullscreen();

    Component inputBox = Input(&state->inputText, "Type your message...");

    auto sendMessage = [&] {
        if (state->inputText.empty()) return;

        std::string userMsg = state->inputText;
        state->history.push_back({"You", userMsg});
        state->inputText.clear();

        // Blocking call for now — same limitation as before
        std::string raw = callOllama(userMsg);
        std::string reply = extractReply(raw);
        state->history.push_back({"Assistant", reply});

        // Snap back to the bottom whenever a new message arrives
        state->scrollOffset = 0;
    };

    inputBox |= CatchEvent([&](Event event) {
        if (event == Event::Return) {
            sendMessage();
            return true;
        }
        return false;
    });

    auto renderer = Renderer(inputBox, [&] {
        Elements messageElements;

        // How many messages to show at once, and which slice given scrollOffset
        const int visibleCount = 12; // tune this to taste, or base it on terminal height
        int total = static_cast<int>(state->history.size());
        int maxOffset = std::max(0, total - visibleCount);
        state->scrollOffset = std::clamp(state->scrollOffset, 0, maxOffset);

        int endIdx = total - state->scrollOffset;
        int startIdx = std::max(0, endIdx - visibleCount);

        for (int i = startIdx; i < endIdx; ++i) {
            auto& msg = state->history[i];
            auto color = (msg.speaker == "You") ? Color::Cyan : Color::Green;
            messageElements.push_back(
                hbox({
                    text(msg.speaker + ": ") | bold | ftxui::color(color),
                    paragraph(msg.text),
                }) | flex
            );
            messageElements.push_back(separator());
        }

        std::string scrollHint = (state->scrollOffset > 0)
            ? "-- scrolled up (" + std::to_string(state->scrollOffset) + ") | PageDown to return to latest --"
            : "";

        return vbox({
            text("AI Assistant (Ollama)") | bold | center,
            separator(),
            vbox(messageElements) | flex | border,
            text(scrollHint) | dim | center,
            separator(),
            hbox({
                text("You: "),
                inputBox->Render(),
            }) | border,
        });
    });

    // Catch scroll-related input (mouse wheel + PageUp/PageDown) at the top level,
    // so it works regardless of which component currently has focus.
    auto withScroll = CatchEvent(renderer, [&](Event event) {
        if (event == Event::PageUp) {
            state->scrollOffset += 3;
            return true;
        }
        if (event == Event::PageDown) {
            state->scrollOffset = std::max(0, state->scrollOffset - 3);
            return true;
        }
        if (event.is_mouse()) {
            auto& mouse = event.mouse();
            if (mouse.button == Mouse::WheelUp) {
                state->scrollOffset += 1;
                return true;
            }
            if (mouse.button == Mouse::WheelDown) {
                state->scrollOffset = std::max(0, state->scrollOffset - 1);
                return true;
            }
        }
        return false;
    });

    screen.Loop(withScroll);

    curl_global_cleanup();
    return 0;
}