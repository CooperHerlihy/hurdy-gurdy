#pragma once

#include "hg/inttypes.hpp"
#include "hg/maybe.hpp"
#include "hg/span.hpp"
#include "hg/smart_ptr.hpp"
#include "hg/gpu.hpp"

namespace hg {

/**
 * Processes all events since startup or the last call to process events
 */
void processEvents();

/**
 * Returns whether the application was quit
 */
bool wasQuit();

/**
 * The types of events
 */
enum WindowEventType : u32 {
    WindowEventType_none = 0,
    WindowEventType_buttonPress,
    WindowEventType_buttonRelease,
    WindowEventType_count,
};

/**
 * The button inputs
 */
enum Button : u32 {
    Button_none = 0,
    Button_0,
    Button_1,
    Button_2,
    Button_3,
    Button_4,
    Button_5,
    Button_6,
    Button_7,
    Button_8,
    Button_9,
    Button_q,
    Button_w,
    Button_e,
    Button_r,
    Button_t,
    Button_y,
    Button_u,
    Button_i,
    Button_o,
    Button_p,
    Button_a,
    Button_s,
    Button_d,
    Button_f,
    Button_g,
    Button_h,
    Button_j,
    Button_k,
    Button_l,
    Button_z,
    Button_x,
    Button_c,
    Button_v,
    Button_b,
    Button_n,
    Button_m,
    Button_semicolon,
    Button_colon,
    Button_apostrophe,
    Button_quotation,
    Button_comma,
    Button_period,
    Button_question,
    Button_grave,
    Button_tilde,
    Button_exclamation,
    Button_at,
    Button_hash,
    Button_dollar,
    Button_percent,
    Button_carot,
    Button_ampersand,
    Button_asterisk,
    Button_lparen,
    Button_rparen,
    Button_lbracket,
    Button_rbracket,
    Button_lbrace,
    Button_rbrace,
    Button_equal,
    Button_less,
    Button_greater,
    Button_plus,
    Button_minus,
    Button_slash,
    Button_backslash,
    Button_underscore,
    Button_bar,
    Button_up,
    Button_down,
    Button_left,
    Button_right,
    Button_mouse1,
    Button_mouse2,
    Button_mouse3,
    Button_mouse4,
    Button_mouse5,
    Button_lmouse = Button_mouse1,
    Button_rmouse = Button_mouse2,
    Button_mmouse = Button_mouse3,
    Button_escape,
    Button_space,
    Button_enter,
    Button_backspace,
    Button_kdelete,
    Button_insert,
    Button_tab,
    Button_home,
    Button_end,
    Button_pageup,
    Button_pagedown,
    Button_f1,
    Button_f2,
    Button_f3,
    Button_f4,
    Button_f5,
    Button_f6,
    Button_f7,
    Button_f8,
    Button_f9,
    Button_f10,
    Button_f11,
    Button_f12,
    Button_lshift,
    Button_rshift,
    Button_lctrl,
    Button_rctrl,
    Button_lalt,
    Button_ralt,
    Button_lsuper,
    Button_rsuper,
    Button_capslock,
    Button_numlock,
    Button_scrolllock,
    Button_pause,
    Button_count,
};

/**
 * Input event data
 */
struct WindowEvent {
    /**
     * The type of event
     */
    WindowEventType type;
    /**
     * The button that was pressed or released
     */
    Button button;
};

/**
 * The present mode for the swapchain
 */
enum GpuPresentMode : u32 {
    GpuPresentMode_immediate = 0,
    GpuPresentMode_mailbox = 1,
    GpuPresentMode_fifo = 2,
    GpuPresentMode_fifoRelaxed = 3,
};

/**
 * Configuration for a window
 */
struct WindowConfig {
    /**
     * How the swapchain images will be presented
     *
     * Note, will fall back to FIFO if preferred is unavailable
     */
    GpuPresentMode preferredPresentMode = GpuPresentMode_fifo;
    /**
     * How the swapchain images will be used
     */
    GpuImageUsageFlags imageUsage = GpuImageUsage_colorAttachment;
};

/**
 * Window implementation data
 */
struct WindowData;

/**
 * A window
 */
struct Window {
    /**
     * Implementation data
     */
    UniquePtr<WindowData> data;

    /**
     * Construct empty
     */
    Window() noexcept;

    /**
     * Open a new window
     */
    static Maybe<Window> create(const WindowConfig& config = {});

    /**
     * Close the window
     */
    ~Window() noexcept;

    /**
     * Set the window title
     */
    void setTitle(StringView title);

    /**
     * Set the width and height
     */
    void setSize(u32 width, u32 height, bool resizeable = true);

    /**
     * Set to fullscreen or disable fullscreen
     */
    void setFullscreen(bool set = true);

    /**
     * Returns the window's pixel format
     */
    Format imageFormat() const;

    /**
     * Returns the window's current image, or nullptr if unavailable this frame
     */
    GpuView* imageView() const;

    /**
     * Returns whether the window was closed
     */
    bool wasClosed() const;

    /**
     * Returns whether the mouse is focused on the window
     */
    bool isFocused() const;

    /**
     * Get the window's width in pixels
     */
    u32 width() const;

    /**
     * Get the window's width in pixels
     */
    u32 height() const;

    /**
     * Returns the current mouse x position relative to the window height
     */
    f32 mouseX() const;

    /**
     * Returns the current mouse y position relative to the window height
     */
    f32 mouseY() const;

    /**
     * Returns the change in mouse x position relative to the window height
     */
    f32 mouseDX() const;

    /**
     * Returns the change in mouse y position relative to the window height
     */
    f32 mouseDY() const;

    /**
     * Returns whether the key is currently down
     */
    bool isButtonDown(Button key) const;

    /**
     * Get the key events since last event processing
     */
    Span<WindowEvent> events() const;

    /**
     * Move construct
     */
    Window(Window&& other) noexcept;

    /**
     * Move assign
     */
    Window& operator=(Window&& other) noexcept;

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
};

/**
 * Acquire an image from each swapchain and begin a command buffer
 *
 * Returns
 * - The command buffer to record this frame
 */
GpuCmd* gpuBeginFrame(Span<Window*> windows);

/**
 * Finishes recording the command buffer and presents the window images
 *
 * Parameters
 * - cmd The command buffer given from beginFrame
 */
void gpuEndFrame(GpuCmd* cmd);

} // namespace hg
