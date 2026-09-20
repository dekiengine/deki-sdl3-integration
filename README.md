# Deki SDL3 Integration

Docs: https://dekiengine.github.io/deki-sdl3-integration/ (components and properties, generated from the code)

SDL3 display, input, and filesystem integration for the Deki Engine.

Part of [Deki Engine](https://github.com/dekiengine/deki-engine).

## Namespace

Types live in `DekiSdl3`. Scene files store the qualified name, and so does code:

```cpp
using namespace DekiSdl3;
obj->AddComponent<SomeComponent>();
```

Scenes saved before 0.16.0 used bare names and still load; saving writes the current one.

## Install

Package Manager in the Deki Editor, or `DekiEditor --packages-add deki-sdl3-integration <project>`.

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

Apache 2.0. See [LICENSE](LICENSE).

Third-party licenses are listed in [NOTICE](NOTICE).
