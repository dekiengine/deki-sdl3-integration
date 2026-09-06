# Deki SDL3 Integration

Documentation: https://dekiengine.github.io/deki-sdl3-integration/ (components and properties, generated from the code)

SDL3 display, input, and filesystem integration for the Deki Engine.

Part of the [Deki Engine](https://github.com/dekiengine/deki-engine) package ecosystem.

## Installation

Install via the Package Manager inside the Deki Editor.

## Dependencies

| Dependency | Type |
|---|---|
| `deki-input` | Deki package |
| `SDL3` (3.2.8) | External (zlib License) |

## Partial present

`SDL3Display::PresentRegions` uploads only the changed rectangles of an RGB565
framebuffer (`SDL_LockTexture` with a rect) when the project's dirty-rect
tracking is on; the window is still redrawn from the texture every frame.
Other framebuffer formats use the whole-frame path.

## License

Licensed under the Apache License, Version 2.0. See [LICENSE](LICENSE) for details.

Third-party licenses are listed in [NOTICE](NOTICE).
