// Minimal xdg-shell window. Unlike layer-shell, this is a normal
// desktop window that Hyprland can move, drag, and manage — we just
// tell Hyprland (via windowrulev2 in hyprland.conf) to always float it.
//
// Build:
//   wayland-scanner client-header /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg-shell-client-protocol.h
//   wayland-scanner private-code  /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg-shell-protocol.c
//   gcc -o wayfl wayfl.c xdg-shell-protocol.c -lwayland-client
//
// Then add to ~/.config/hypr/hyprland.conf:
//   windowrulev2 = float, class:^(wayfl)$
//   windowrulev2 = size 320 240, class:^(wayfl)$
//   windowrulev2 = move 100 100, class:^(wayfl)$   (optional starting position)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"

static struct wl_compositor *compositor = NULL;
static struct wl_shm *shm = NULL;
static struct xdg_wm_base *wm_base = NULL;

static struct wl_surface *surface = NULL;
static struct xdg_surface *xdg_surface = NULL;
static struct xdg_toplevel *xdg_toplevel = NULL;

static const int WIDTH = 320;
static const int HEIGHT = 240;

// --- ping/pong keeps the compositor from thinking we're frozen ---
static void wm_base_ping(void *data, struct xdg_wm_base *wm_base, uint32_t serial) {
    xdg_wm_base_pong(wm_base, serial);
}
static const struct xdg_wm_base_listener wm_base_listener = { .ping = wm_base_ping };

// --- registry: grab globals ---
static void registry_global(void *data, struct wl_registry *registry,
                             uint32_t id, const char *interface, uint32_t version) {
    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 4);
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        wm_base = wl_registry_bind(registry, id, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(wm_base, &wm_base_listener, NULL);
    }
}
static void registry_remove(void *data, struct wl_registry *registry, uint32_t id) {}
static const struct wl_registry_listener registry_listener = {
    .global = registry_global,
    .global_remove = registry_remove,
};

// --- fill a shm buffer with a solid color ---
static struct wl_buffer *make_buffer(uint32_t color) {
    char template[] = "/tmp/wayfl-XXXXXX";
    int fd = mkstemp(template);
    unlink(template);
    size_t size = WIDTH * HEIGHT * 4;
    ftruncate(fd, size);
    uint32_t *pixels = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    for (int i = 0; i < WIDTH * HEIGHT; i++) pixels[i] = color;

    struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);
    struct wl_buffer *buffer = wl_shm_pool_create_buffer(
        pool, 0, WIDTH, HEIGHT, WIDTH * 4, WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);
    munmap(pixels, size);
    close(fd);
    return buffer;
}

// --- xdg_surface configure: compositor says "ready, draw now" ---
static void xdg_surface_configure(void *data, struct xdg_surface *xs, uint32_t serial) {
    xdg_surface_ack_configure(xs, serial);
    struct wl_buffer *buf = make_buffer(0xFF2E2E3A); // opaque dark slate
    wl_surface_attach(surface, buf, 0, 0);
    wl_surface_commit(surface);
}
static const struct xdg_surface_listener xdg_surface_listener_impl = {
    .configure = xdg_surface_configure,
};

// --- toplevel events ---
static void toplevel_configure(void *data, struct xdg_toplevel *tl,
                                int32_t width, int32_t height, struct wl_array *states) {
    // Compositor may suggest a size; we ignore it here and keep fixed size.
}
static void toplevel_close(void *data, struct xdg_toplevel *tl) {
    exit(0);
}
static const struct xdg_toplevel_listener toplevel_listener = {
    .configure = toplevel_configure,
    .close = toplevel_close,
};

int main() {
    struct wl_display *display = wl_display_connect(NULL);
    if (!display) {
        fprintf(stderr, "failed to connect to Wayland display\n");
        return 1;
    }

    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);
    wl_display_roundtrip(display);

    if (!compositor || !shm || !wm_base) {
        fprintf(stderr, "missing required Wayland globals\n");
        return 1;
    }

    surface = wl_compositor_create_surface(compositor);
    xdg_surface = xdg_wm_base_get_xdg_surface(wm_base, surface);
    xdg_surface_add_listener(xdg_surface, &xdg_surface_listener_impl, NULL);

    xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);
    xdg_toplevel_add_listener(xdg_toplevel, &toplevel_listener, NULL);

    // app_id is what Hyprland's windowrulev2 "class:" matches against
    xdg_toplevel_set_app_id(xdg_toplevel, "wayfl");
    xdg_toplevel_set_title(xdg_toplevel, "AI Assistant");

    wl_surface_commit(surface);

    while (wl_display_dispatch(display) != -1) {
        // event loop
    }

    return 0;
}