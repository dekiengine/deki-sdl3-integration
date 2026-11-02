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

## Presenting through OpenGL

With `deki-gl` in the project, `SDL3DisplaySetup` opens an OpenGL window
instead (`SDL3GLDisplay`), presented by `deki-gl`'s presenter: the frame is
built on the GPU, and `deki-3d-gl` draws meshes straight into it rather than
reading them back. The context is OpenGL ES 3.0 on Linux, the API a handheld
has, and desktop OpenGL 3.3 core elsewhere. The frame buffer is then RGB565A8,
whatever the platform's screen format, so the 2D can be composed in layers.

`presentThroughOpenGL` turns it off. Without `deki-gl`, or if OpenGL does not
come up, the window is `SDL3Display` over SDL's GPU API as before.

## Community

Questions, ideas and what you made with Deki: [the Deki Discord](https://dekiengine.com/discord/).

## License

Apache 2.0. See [LICENSE](LICENSE).

Third-party licenses are listed in [NOTICE](NOTICE).
