# PineNote Drawing Application with LLM Integration

## Project Overview

This document outlines the design and implementation considerations for a
drawing application targeting the Pine64 PineNote e-ink tablet. The application
features a configurable toolbar for LLM-powered prompts, enabling users to draw
content (e.g., tables, diagrams) and convert it to structured text formats via
network-based LLM inference.

---

## Hardware Platform

### Pine64 PineNote Specifications

| Component        | Details                                                        |
| ---------------- | -------------------------------------------------------------- |
| **SoC**          | Rockchip RK3566 (4x ARM Cortex-A55, up to 1.8GHz)              |
| **Display**      | 10.3" E-ink ED103TC2 (1872×1404, 227 DPI, 16 grayscale levels) |
| **Stylus**       | Wacom W9013 (HID-over-I2C) + WS8100 BLE pen                    |
| **Touch**        | Cypress TT21000 (multi-touch with pressure sensitivity)        |
| **RAM**          | 4GB                                                            |
| **Storage**      | 128GB eMMC                                                     |
| **Connectivity** | WiFi (BCM43438), Bluetooth                                     |
| **OS**           | Debian Linux (community-built)                                 |

### Display Characteristics

- **Resolution**: 1872 × 1404 pixels
- **Color Depth**: 4-bit grayscale (16 levels)
- **Refresh Rate**: Variable depending on waveform mode
- **Panel**: E-Ink ED103TC2 with DPI interface

### Stylus Input Capabilities

The PineNote implements dual stylus systems:

1. **Wacom W9013 Digitizer** (Primary)
   - HID-over-I2C protocol
   - Pressure sensitivity
   - Tilt detection (X/Y axes)
   - Button support (BTN_STYLUS, BTN_STYLUS2)
   - Eraser mode (BTN_TOOL_RUBBER)

2. **Pine64 WS8100 BLE Pen** (Secondary)
   - Wireless Bluetooth Low Energy
   - Multiple button actions
   - Battery monitoring
   - Custom SPI-GPIO driver

---

## System Architecture

### Component Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                  Rust Core (async main loop)                │
├─────────────┬─────────────────────┬─────────────────────────┤
│ StylusStream│   LVGL FFI          │   HttpFuture            │
│ (Stream)    │   (lv_canvas)       │   (Future)              │
├─────────────┼─────────────────────┼─────────────────────────┤
│ stylus.c    │   LVGL C library    │   ureq (thread)         │
│ (libevdev)  │   (SDL/DRM backend) │                         │
├─────────────┼─────────────────────┼─────────────────────────┤
│ /dev/input/ │  /dev/dri/card0     │   LLM API endpoint      │
│ eventX      │  + custom ioctls    │                         │
└─────────────┴─────────────────────┴─────────────────────────┘
```

### Data Flow

1. **Stylus Input**: `StylusStream` wraps C FFI (libevdev) → async events via
   callback
2. **Display Output**: LVGL `lv_canvas` → DRM/KMS framebuffer → kernel EBC
   driver handles e-ink refresh
3. **LLM Integration**: Toolbar action → `HttpFuture` spawns thread → blocking
   `ureq` call → result via channel

### Key Design Points

- The application does not implement display driver logic directly
- Framebuffer writes go through standard DRM interfaces
- The kernel's EBC driver (`rockchip_ebc.c`) manages e-ink waveforms and panel
  refresh
- Custom ioctls provide fine-grained control over refresh behavior

---

## Kernel Driver Interface

### E-ink Display Driver

The PineNote uses a DRM/KMS-based e-ink controller driver with the following
components:

| File                                                | Purpose                                        |
| --------------------------------------------------- | ---------------------------------------------- |
| `drivers/gpu/drm/rockchip/rockchip_ebc.c`           | Primary EBC controller driver (2527 lines)     |
| `drivers/gpu/drm/rockchip/rockchip_ebc.h`           | Driver header and data structures              |
| `drivers/gpu/drm/rockchip/rockchip_ebc_blit_neon.c` | ARM NEON optimized pixel blitting (1253 lines) |
| `include/uapi/drm/rockchip_ebc_drm.h`               | Userspace ioctl definitions                    |
| `drivers/gpu/drm/drm_epd_helper.c`                  | Generic EPD waveform/LUT handling              |

### Custom IOCTLs

The driver exposes the following custom ioctls for userspace control:

| IOCTL                                   | Purpose                                          |
| --------------------------------------- | ------------------------------------------------ |
| `DRM_IOCTL_ROCKCHIP_EBC_GLOBAL_REFRESH` | Trigger full-screen refresh                      |
| `DRM_IOCTL_ROCKCHIP_EBC_OFF_SCREEN`     | Update off-screen buffer                         |
| `DRM_IOCTL_ROCKCHIP_EBC_EXTRACT_FBS`    | Extract internal framebuffers                    |
| `DRM_IOCTL_ROCKCHIP_EBC_RECT_HINTS`     | Set pixel hints for rectangular regions          |
| `DRM_IOCTL_ROCKCHIP_EBC_MODE`           | Query/set driver mode, dither mode, redraw delay |
| `DRM_IOCTL_ROCKCHIP_EBC_ZERO_WAVEFORM`  | Enable/disable zero waveform mode                |
| `DRM_IOCTL_ROCKCHIP_EBC_PHASE_SEQUENCE` | Configure custom phase sequences                 |

### Waveform Modes

E-ink displays require different waveforms for different use cases:

| Waveform    | Use Case                 | Characteristics                        |
| ----------- | ------------------------ | -------------------------------------- |
| A2          | Fast drawing, animations | Black/white only, fastest refresh      |
| DU          | Quick grayscale updates  | 16-level to monochrome                 |
| DU4         | Medium quality           | 16-level to 4-level grayscale          |
| GC16        | High quality display     | Full 16-level grayscale, visible flash |
| GL16        | Reading, static content  | Less flash than GC16                   |
| GLR16/GLD16 | Anti-ghosting            | Reduced artifacts                      |

### Driver Modes

```c
ROCKCHIP_EBC_DRIVER_MODE_NORMAL         // Standard waveform display
ROCKCHIP_EBC_DRIVER_MODE_FAST           // Shortened waveforms
ROCKCHIP_EBC_DRIVER_MODE_ZERO_WAVEFORM  // Direct pixel control
ROCKCHIP_EBC_DRIVER_MODE_PHASE_SEQUENCE // Custom phase-based updates
```

### Stylus Input Driver

| File                              | Purpose                                   |
| --------------------------------- | ----------------------------------------- |
| `drivers/hid/i2c-hid/`            | Generic HID-over-I2C driver (Wacom W9013) |
| `drivers/input/misc/ws8100-pen.c` | Pine64 BLE pen driver                     |

Input events conform to Linux input subsystem standards:

```c
// Relevant input event codes
ABS_X, ABS_Y          // Position
ABS_PRESSURE          // Pressure (0-4095 typical)
ABS_TILT_X, ABS_TILT_Y // Tilt angles
BTN_TOOL_PEN          // Pen in proximity
BTN_TOOL_RUBBER       // Eraser mode
BTN_TOUCH             // Tip contact
BTN_STYLUS            // Side button 1
BTN_STYLUS2           // Side button 2
```

---

## Key Finding: Kernel Handles E-ink Complexity

Analysis of existing PineNote applications (Xournal++, KOReader, Foliate)
reveals that **none implement their own e-ink waveform management**. All rely
entirely on the kernel's `rockchip_ebc.c` driver.

### How Existing Apps Work

| Application   | E-ink Library | Actual Implementation                          |
| ------------- | ------------- | ---------------------------------------------- |
| **Xournal++** | None          | Standard GTK3 → DRM → kernel handles waveforms |
| **KOReader**  | None          | Lua/C → framebuffer → kernel handles waveforms |
| **Foliate**   | None          | GTK4/WebKit → DRM → kernel handles waveforms   |

### Why FBInk Is Not Used

[FBInk](https://github.com/NiLuJe/FBInk) is a popular e-ink framebuffer library
for Kindle/Kobo/reMarkable devices. However:

- PineNote uses **DRM/KMS** (not legacy framebuffer)
- The kernel's `rockchip_ebc.c` driver already handles all e-ink specifics
- FBInk does not support Rockchip EBC hardware

### Implications for This Project

The kernel driver handles:

- Waveform selection (A2, GC16, GL16, etc.)
- Temperature compensation via IIO sensor
- LUT (Look-Up Table) management
- Ghosting prevention
- NEON-optimized pixel blitting

**Application responsibility is minimal:**

```
┌─────────────────────────────────────────────────────────────┐
│            Drawing Application (LVGL + Rust FFI)            │
│              (lv_canvas for drawing surface)                │
├─────────────────────────────────────────────────────────────┤
│          Optional: Thin ioctl wrapper (~50 lines)           │
│   - DRM_IOCTL_ROCKCHIP_EBC_MODE (set fast mode)             │
│   - DRM_IOCTL_ROCKCHIP_EBC_RECT_HINTS (partial refresh)     │
├─────────────────────────────────────────────────────────────┤
│          Kernel: rockchip_ebc.c (2527 lines)                │
│   - All e-ink complexity handled here                       │
└─────────────────────────────────────────────────────────────┘
```

---

## Complexity Analysis (Revised)

| Component                  | Difficulty  | Rationale                                                 |
| -------------------------- | ----------- | --------------------------------------------------------- |
| Drawing primitives         | Medium      | Standard framebuffer rendering; libraries available       |
| E-ink refresh optimization | **Low-Med** | Kernel handles waveforms; app just calls ioctls for hints |
| Low-latency pen tracking   | Medium      | Use FAST driver mode during drawing; kernel optimizes     |
| Toolbar/UI                 | Low         | Standard UI toolkit implementation                        |
| LLM network integration    | Low         | HTTP POST with JSON payload                               |

### Remaining Technical Challenges

1. **Driver Mode Switching**
   - Call `DRM_IOCTL_ROCKCHIP_EBC_MODE` to set `ROCKCHIP_EBC_DRIVER_MODE_FAST`
     during active drawing
   - Switch back to `ROCKCHIP_EBC_DRIVER_MODE_NORMAL` when idle
   - Implement simple state machine (drawing vs. idle timeout)

2. **Partial Refresh Hints**
   - Track bounding box of stylus strokes
   - Send dirty rectangles via `DRM_IOCTL_ROCKCHIP_EBC_RECT_HINTS`
   - Let kernel optimize the actual refresh

3. **Drawing Canvas Implementation**
   - Use LVGL's `lv_canvas` widget for stroke rendering
   - Built-in primitives: lines, arcs, rectangles, individual pixels

---

## UI Toolkit: LVGL

**Repository**: https://github.com/lvgl/lvgl

### Why LVGL

| Feature                 | Benefit for This Project                                    |
| ----------------------- | ----------------------------------------------------------- |
| `lv_canvas` widget      | Native drawing surface with lines, arcs, pixels, rectangles |
| SDL simulator           | Desktop development without target hardware                 |
| Mature C library        | Stable, well-documented, production-proven                  |
| Lightweight             | Minimal footprint for simple toolbar UI                     |
| DRM/framebuffer support | Linux display backends available                            |

### Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Rust Application                         │
│  ┌─────────────────┐  ┌──────────────────────────────────┐  │
│  │  Core Logic     │  │  Thin C FFI Wrappers             │  │
│  │  - Stroke data  │  │  - stylus.c (libevdev)           │  │
│  │  - LLM client   │  │  - lv_*.c (LVGL)                 │  │
│  │  - async loop   │  │  - Event dispatch                │  │
│  └─────────────────┘  └──────────────────────────────────┘  │
├─────────────────────────────────────────────────────────────┤
│  libevdev          │  LVGL (C library)                      │
│                    │  - lv_canvas for drawing               │
│                    │  - SDL driver (dev) / DRM (prod)       │
└────────────────────┴────────────────────────────────────────┘
```

### Simulator Support

LVGL's SDL backend enables full desktop development. Input abstraction via Rust
trait allows swapping `StylusStream` (prod, libevdev) with `MouseStream` (dev,
SDL events).

### Alternative Consideration

> **Note**: If the UI grows significantly beyond a simple toolbar (complex
> widgets, animations, multi-screen navigation), consider migrating to
> [Slint](https://slint.dev/) for its native Rust implementation and declarative
> UI language.

---

## Existing Software

### PineNote Ecosystem

| Application   | Description                    | Status                                     |
| ------------- | ------------------------------ | ------------------------------------------ |
| **Xournal++** | Note-taking and PDF annotation | Preinstalled on PineNote Community Edition |
| **KOReader**  | E-book reader with annotation  | Available                                  |
| **Foliate**   | E-book reader                  | Preinstalled                               |

### Kernel Repository Contents

This repository (`hrdl-linux`) contains kernel source only. No userspace drawing
applications are included.

**Relevant kernel samples**:

- `samples/auxdisplay/cfag12864b-example.c` — Simple LCD drawing demo (not
  applicable to this project)

### Implementation Strategy

**Recommended: New Rust + LVGL Implementation**

Forking Xournal++ is impractical because:

- C++/GTK3 codebase doesn't integrate well with Rust
- Adding LLM features requires significant refactoring
- Input handling is tightly coupled to GTK

A new LVGL-based implementation offers:

- Core logic in Rust, thin C FFI for LVGL and stylus input
- `lv_canvas` for drawing with simulator support
- Lightweight async without tokio (custom Stream/Future, `futures-lite`)
- Direct libevdev input via minimal C wrapper
- Minimal dependencies suitable for embedded

---

## C FFI Layer

The application uses thin C FFI wrappers for both display (LVGL) and input
(libevdev). This avoids heavy Rust binding crates while maintaining a clean Rust
core.

### Stylus Input FFI

The PineNote has two input devices for stylus input, both managed via libevdev:

#### Input Devices

```
/dev/input/event*
├── Wacom W9013 Digitizer (I2C)
│   Position, pressure, tilt, basic buttons via EMR
│
└── WS8100 BLE Pen (SPI) [optional]
    Extra buttons, macros via Bluetooth
```

#### Wacom W9013 — EMR Digitizer

The Wacom digitizer uses electromagnetic resonance (EMR) to detect the pen. The
panel emits an EM field; the pen contains passive resonant circuits that
modulate the field based on position, pressure, and button state. No battery
required in the pen for basic functionality.

| Event    | Code                       | Description                           |
| -------- | -------------------------- | ------------------------------------- |
| `EV_ABS` | `ABS_X`, `ABS_Y`           | Pen position (absolute coordinates)   |
| `EV_ABS` | `ABS_PRESSURE`             | Tip pressure (0-4095 typical)         |
| `EV_ABS` | `ABS_TILT_X`, `ABS_TILT_Y` | Pen tilt angle                        |
| `EV_KEY` | `BTN_TOUCH`                | Tip contact with surface              |
| `EV_KEY` | `BTN_TOOL_PEN`             | Pen in proximity                      |
| `EV_KEY` | `BTN_TOOL_RUBBER`          | Eraser end in proximity               |
| `EV_KEY` | `BTN_STYLUS`               | Side button 1 (detected via EM field) |
| `EV_KEY` | `BTN_STYLUS2`              | Side button 2 (detected via EM field) |

Button presses on the pen barrel modulate the EMR signal — the digitizer "sees"
them electromagnetically, no wireless protocol needed.

#### WS8100 BLE Pen — Wireless Buttons

Pine64's active pen with Bluetooth radio. Provides additional buttons and
battery monitoring not possible through EMR. Works alongside the W9013 — the
same pen is detected by both drivers simultaneously.

| Event    | Code                                       | Description          |
| -------- | ------------------------------------------ | -------------------- |
| `EV_KEY` | `BTN_STYLUS`, `BTN_STYLUS2`, `BTN_STYLUS3` | Pen buttons          |
| `EV_KEY` | `KEY_BACK`, `KEY_FORWARD`, `KEY_SLEEP`     | Long-press functions |
| `EV_KEY` | `KEY_MACRO1`, `KEY_MACRO2`, `KEY_MACRO3`   | Programmable macros  |

#### FFI Design

The C layer will open both input devices (W9013 required, WS8100 optional), poll
their file descriptors, and deliver merged events via callback. The Rust layer
registers a callback and constructs an async `Stream` from the events.

**Build dependency**: `libevdev-dev` (system package)

### LVGL FFI

Wrap only the required LVGL functions:

| Category    | Functions                                                                                            |
| ----------- | ---------------------------------------------------------------------------------------------------- |
| **Init**    | `lv_init()`, `lv_timer_handler()`                                                                    |
| **Canvas**  | `lv_canvas_create()`, `lv_canvas_set_buffer()`, `lv_canvas_init_layer()`, `lv_canvas_finish_layer()` |
| **Drawing** | `lv_draw_line()`, `lv_draw_arc()`, `lv_canvas_set_px()`                                              |
| **Widgets** | `lv_dropdown_create()`, `lv_btn_create()` (toolbar)                                                  |
| **Display** | `lv_sdl_window_create()` (dev), DRM setup (prod)                                                     |

---

## Async Pattern (No Tokio)

The application uses Rust's async/await ergonomics without the tokio runtime
overhead. Custom `Stream` and `Future` implementations wrap the C FFI layer.

### Architecture

```
┌─────────────────────────────────────────────────────────────┐
│              futures_lite::future::block_on                 │
├─────────────────────────────────────────────────────────────┤
│                    async fn main_loop()                     │
│  - StylusStream: impl Stream (wraps stylus FFI callback)    │
│  - HttpFuture: impl Future (wraps blocking ureq in thread)  │
│  - LVGL tick via lv_timer_handler()                         │
└─────────────────────────────────────────────────────────────┘
```

### Key Concepts

| Component         | Implementation                                                              |
| ----------------- | --------------------------------------------------------------------------- |
| **Executor**      | `futures_lite::future::block_on()` — single-threaded, minimal               |
| **Stylus Stream** | Custom `Stream` impl receiving events from C callback                       |
| **HTTP Future**   | Spawns `std::thread`, runs blocking `ureq` call, returns result via channel |
| **Main Loop**     | `select!` over stylus events and HTTP responses, then tick LVGL             |

### Why Not Tokio

| Aspect       | This Design | Tokio                   |
| ------------ | ----------- | ----------------------- |
| Binary size  | +200KB      | +5-10MB                 |
| Compile time | Fast        | Slow                    |
| Complexity   | Simple      | Runtime, spawners, etc. |
| Dependencies | 3-4 crates  | 20+ transitive          |

For a single-user embedded app with light HTTP traffic, the simple approach is
sufficient.

---

## LLM Integration

HTTP requests run on spawned threads with results delivered via `Future`. Uses
`ureq` (blocking HTTP client) and `nanoserde` (minimal JSON serialization).

### Request Flow

1. User triggers toolbar action
2. Canvas exported to PNG bytes
3. PNG base64-encoded, wrapped in vision API request JSON
4. Thread spawned with blocking `ureq::post()`
5. `HttpFuture` completes when thread returns
6. Response parsed, result displayed

### Serialization

Using `nanoserde` for minimal footprint (also `no_std` compatible for other
projects):

| Crate                  | Size   | `no_std` | Notes                                     |
| ---------------------- | ------ | -------- | ----------------------------------------- |
| `serde` + `serde_json` | ~300KB | Partial  | Full-featured, standard                   |
| `nanoserde`            | ~15KB  | Yes      | Derive macros, sufficient for simple JSON |

---

## Dependencies

```toml
[dependencies]
# Async (lightweight, no tokio)
futures-core = "0.3"      # Stream/Future traits
futures-lite = "2"        # block_on executor, combinators

# HTTP (blocking, runs in thread)
ureq = "2"

# Serialization (minimal)
nanoserde = "0.1"
base64 = "0.22"

[build-dependencies]
cc = "1"                  # Compile stylus.c and LVGL wrapper
```

### System Dependencies

- `libevdev-dev` — stylus input
- `libsdl2-dev` — LVGL simulator (dev only)

---

## Additional Considerations

- **Stroke Stabilization**: Xournal++ uses a `StrokeStabilizer` class for
  smoothing. Consider moving average or bezier fitting for drawing quality.
- **Device Hot-plug**: Use `udev` or `inotify` to detect stylus
  connect/disconnect.
- **Input Abstraction**: Rust trait allows swapping `StylusInput` (prod) with
  `MouseInput` (dev simulator).

---

## Key Kernel Files Reference

| Purpose                 | Path                                                |
| ----------------------- | --------------------------------------------------- |
| E-ink display driver    | `drivers/gpu/drm/rockchip/rockchip_ebc.c`           |
| Userspace ioctls        | `include/uapi/drm/rockchip_ebc_drm.h`               |
| NEON-optimized blitting | `drivers/gpu/drm/rockchip/rockchip_ebc_blit_neon.c` |
| EPD helper library      | `drivers/gpu/drm/drm_epd_helper.c`                  |
| EPD helper header       | `include/drm/drm_epd_helper.h`                      |
| Wacom digitizer         | `drivers/hid/i2c-hid/` (generic HID-over-I2C)       |
| BLE pen driver          | `drivers/input/misc/ws8100-pen.c`                   |
| Device tree             | `arch/arm64/boot/dts/rockchip/rk3566-pinenote.dtsi` |
| Kernel config           | `arch/arm64/configs/pinenote_defconfig`             |

---

## References

### Hardware & Development

- [Pine64 PineNote Wiki](https://wiki.pine64.org/wiki/PineNote)
- [PineNote Development Documentation](https://pine64.org/documentation/PineNote/Development/)
- [RK3566 EBC Reverse-Engineering](https://wiki.pine64.org/wiki/RK3566_EBC_Reverse-Engineering)
- [E-ink Dev Notes (rmkit)](https://rmkit.dev/eink-dev-notes/)

### E-ink Libraries (For Reference)

- [FBInk - FrameBuffer eInker](https://github.com/NiLuJe/FBInk) — Note: Does not
  support PineNote/Rockchip EBC

### UI Toolkit

- [LVGL Documentation](https://docs.lvgl.io/master/)
- [LVGL Canvas Widget](https://docs.lvgl.io/master/widgets/canvas.html)
- [LVGL SDL Driver](https://docs.lvgl.io/master/integration/driver/sdl.html)

### Existing PineNote Applications

- [Xournal++](https://github.com/xournalpp/xournalpp) — GTK3/C++ note-taking app
  (uses GdkDevice for stylus)
- [KOReader](https://github.com/koreader/koreader) — Lua/C e-book reader (raw
  evdev via FFI)
- [Foliate](https://github.com/johnfactotum/foliate) — GTK4/GJS e-book reader
  (no stylus support)

### Input

- [libevdev Documentation](https://www.freedesktop.org/software/libevdev/doc/latest/)
  — C library for evdev (used via FFI)
- [libinput Tablet Documentation](https://wayland.freedesktop.org/libinput/doc/latest/tablet-support.html)

### Async & Serialization (Rust)

- [futures-lite](https://docs.rs/futures-lite) — Lightweight futures/streams,
  `block_on` executor
- [nanoserde](https://docs.rs/nanoserde) — Minimal JSON serialization (~15KB,
  `no_std` compatible)
- [ureq](https://docs.rs/ureq) — Blocking HTTP client

### News & Availability

- [PineNote Pre-Orders - Good e-Reader](https://goodereader.com/blog/electronic-readers/pinenote-pre-orders-currently-open-shipping-starts-mid-november)
- [PineNote Community Edition - Liliputing](https://liliputing.com/pinenote-community-edition-is-a-399-e-ink-tablet-that-ships-with-debian-linux/)

---

## Competitive Landscape

### Commercial E-ink Tablets with AI

| Product                                                         | AI Features                                            | Limitations                                                                                                                                    |
| --------------------------------------------------------------- | ------------------------------------------------------ | ---------------------------------------------------------------------------------------------------------------------------------------------- |
| [Viwoods AiPaper](https://viwoods.com/products/viwoods-aipaper) | Handwriting→text (3 modes), AI Q&A, custom prompts     | Requires internet, 5 page limit, chat UI awkward for OCR, [accuracy issues](https://www.techradar.com/tablets/ereaders/viwoods-aipaper-review) |
| [Kindle Scribe](https://www.amazon.com/dp/B0BX9D88VR)           | AI search/summarize notebooks, handwriting→text export | Cloud-only, hit-or-miss accuracy                                                                                                               |
| [Boox Note Series](https://shop.boox.com/)                      | "Note AI" handwriting recognition, multi-language OCR  | OCR-focused, no generative AI prompts                                                                                                          |
| [reMarkable Paper Pro](https://remarkable.com/)                 | MyScript OCR, cloud sync                               | Subscription required ($2.99/mo), no LLM integration                                                                                           |
| [Supernote](https://supernote.com/)                             | Background handwriting recognition                     | Real-time notebooks only, limited export                                                                                                       |
| [QuietLLM](https://quietllm.com/)                               | ChatGPT for Kindle browser                             | $49, web wrapper only, no drawing/stylus integration                                                                                           |

### Open Source AI Note-Taking (Desktop/Web)

No open source e-ink native apps with LLM integration exist. Desktop
alternatives:

| Project                                                   | Description                                                                |
| --------------------------------------------------------- | -------------------------------------------------------------------------- |
| [Reor](https://github.com/reorproject/reor)               | Local LLM note-taking, Obsidian-like, semantic search, runs Ollama locally |
| [Rocketnotes](https://github.com/fynnfluegge/rocketnotes) | Markdown + AI chat/completion, agentic archiving, Docker/Ollama support    |
| [Open Notebook](https://github.com/lfnovo/open-notebook)  | NotebookLM alternative, multi-modal (PDF, video, audio), 16+ AI providers  |

### Feature Comparison

| Feature                   | This Project   | Viwoods AiPaper   | Kindle Scribe | reMarkable |
| ------------------------- | -------------- | ----------------- | ------------- | ---------- |
| Drawing → structured text | Yes            | Partial (chat UI) | No            | No         |
| Custom prompts            | Yes (toolbar)  | Yes (chat)        | No            | No         |
| Self-hostable             | Yes            | No                | No            | No         |
| Open source               | Yes            | No                | No            | No         |
| Open hardware             | Yes (PineNote) | No                | No            | No         |
| Subscription required     | No             | No\*              | No            | Yes        |

\*Viwoods pays API costs with no subscription —
[sustainability unclear](https://www.splitbrain.org/blog/2025-09/22-viwoods_ai_paper_review)

### Related Reading

- [The Perfect Knowledge Assistant That Does Not Exist](https://preslav.me/2024/01/19/the-perfect-knowledge-assistant-device-that-does-not-exist/)
  — Blog post describing the desired device
- [SkipWriter: LLM-Powered Abbreviated Writing](https://research.google/pubs/skipwriter-llm-powered-abbreviated-writing-on-tablets/)
  — Google Research on LLM + stylus input
- [Viwoods AiPaper Review (Splitbrain)](https://www.splitbrain.org/blog/2025-09/22-viwoods_ai_paper_review)
  — Detailed technical review with AI critique
