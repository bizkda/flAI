# wayfl

A minimal floating Wayland window, built with `xdg-shell`, intended as the base for a lightweight floating AI assistant widget on Hyprland (or any Wayland compositor).

## Requirements

- A Wayland compositor (tested on Hyprland)
- `wayland-client` dev libraries
- `wayland-scanner` + the `xdg-shell` protocol XML (from `wayland-protocols`)
- `gcc` and `make`

### Install dependencies

**Arch / Hyprland-based distros:**
```bash
sudo pacman -S wayland wayland-protocols base-devel
```

**Debian / Ubuntu:**
```bash
sudo apt install libwayland-dev wayland-protocols build-essential
```

**Fedora:**
```bash
sudo dnf install wayland-devel wayland-protocols-devel gcc make
```

## Build

```bash
make
```

This generates the required `xdg-shell` protocol bindings automatically and builds the binary to `bin/wayfl`.

## Run

```bash
make run
```
or directly:
```bash
./bin/wayfl
```

## Make it float on Hyprland

By default, Hyprland tiles new windows. Add a window rule so this app always opens floating, centered, at a fixed size.

Add to your Hyprland config (commonly `~/.config/hypr/config/rules.conf` or `hyprland.conf`):

```
windowrule {
    name = "ai_assistant"
    match:class = ^(wayfl)$
    float = on
    center = on
    size = 320 240
    no_shadow = on
}
```

Then reload Hyprland:
```bash
hyprctl reload
```

> Note: if your Hyprland version uses the older `windowrulev2` syntax instead of block syntax, use:
> ```
> windowrulev2 = float, class:^(wayfl)$
> windowrulev2 = size 320 240, class:^(wayfl)$
> ```

If the rule doesn't match, run `hyprctl clients` while the app is open to confirm the exact `class` Hyprland reports, and adjust the rule accordingly.

## Clean

```bash
make clean
```

Removes the binary and generated protocol files.

## Status

Early scaffold — currently renders a solid-color placeholder window. Intended next steps: draggable dragging support, custom rendering (text/UI), and the actual assistant logic.