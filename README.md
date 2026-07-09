# AI Assistant TUI (Ollama + FTXUI)

A terminal-based chat interface for talking to a local [Ollama](https://ollama.com) model, built in C++ with [FTXUI](https://github.com/ArthurSonzogni/ftxui).

![status](https://img.shields.io/badge/status-early--prototype-yellow)

## Features

- Terminal UI chat window (input box, scrollable message history)
- Talks to a local Ollama server via its REST API (`/api/chat`)
- Color-coded messages (you vs. assistant)

## Requirements

- Linux
- A C++17 compiler (`g++` or `clang++`)
- [Ollama](https://ollama.com) installed and running locally
- Libraries:
  - [FTXUI](https://github.com/ArthurSonzogni/ftxui)
  - `libcurl`
  - [nlohmann/json](https://github.com/nlohmann/json)

## Setup

### 1. Install Ollama and pull a model

```bash
# make sure the Ollama server is running
ollama serve

# in another terminal, pull the model referenced in main.cpp
ollama pull minimax-m3:cloud
```

> If you'd rather use a different model, change the `"model"` field in `callOllama()` inside `main.cpp`, and make sure you've pulled it with `ollama pull <model-name>`.

### 2. Install dependencies

```bash
sudo apt install libcurl4-openssl-dev nlohmann-json3-dev
```

### 3. Install FTXUI

```bash
git clone https://github.com/ArthurSonzogni/ftxui.git
cd ftxui && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

## Build

From the project folder (where `main.cpp` lives):

```bash
g++ main.cpp -o assistant \
    -lftxui-component -lftxui-dom -lftxui-screen \
    -lcurl -std=c++17
```

## Run

Make sure the Ollama server is running (`ollama serve`, or it's already running in the background), then:

```bash
./assistant
```

Type a message and press **Enter** to send it. The assistant's reply will appear below your message.

## Known Limitations

This is an early prototype. A few things are intentionally simplified for now:

- **Blocking calls** — the UI freezes while waiting for a response from Ollama, since the request runs synchronously on the main thread.
- **No streaming** — replies are shown all at once, rather than token-by-token as they're generated.
- **No conversation memory** — each message is sent to Ollama on its own; previous turns aren't included, so the model has no context of earlier messages.

## Planned Improvements

- [ ] Move Ollama requests to a background thread so the UI stays responsive
- [ ] Enable streaming responses for a live "typing" effect
- [ ] Maintain full conversation history and send it with each request
- [ ] Configurable model name (via CLI flag or config file)

## Project Structure

```
.
├── main.cpp     # application source
└── README.md    # this file
```

## License

MIT (or update as you see fit)
