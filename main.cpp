// main.cpp
#include <ftxui/component/captured_mouse.hpp>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

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

        // Blocking call for now — same limitation as the GUI version
        std::string raw = callOllama(userMsg);
        std::string reply = extractReply(raw);
        state->history.push_back({"Assistant", reply});
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
        for (auto& msg : state->history) {
            auto color = (msg.speaker == "You") ? Color::Cyan : Color::Green;
            messageElements.push_back(
                hbox({
                    text(msg.speaker + ": ") | bold | ftxui::color(color),
                    paragraph(msg.text),
                }) | flex
            );
            messageElements.push_back(separator());
        }

        return vbox({
            text("AI Assistant (Ollama)") | bold | center,
            separator(),
            vbox(messageElements) | flex | frame | border,
            separator(),
            hbox({
                text("You: "),
                inputBox->Render(),
            }) | border,
        });
    });

    screen.Loop(renderer);

    curl_global_cleanup();
    return 0;
}
