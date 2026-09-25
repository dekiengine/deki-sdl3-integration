# Changelog

Notable changes to `deki-sdl3-integration`. Engine and editor changes are in the
[engine changelog](https://github.com/dekiengine/deki-engine/blob/master/CHANGELOG.md).

A package's `minEngine` names the engine version it needs. Before 1.0 a
breaking change bumps the minor across the editor, the engine and every
package together, so a package with no changes of its own is still released
alongside one that has them.

## Unreleased

### Changed
- The simulator window emulates the platform's pixel format too
  (`DEKI_SCREEN_COLOR_FORMAT`), which the display reports to the engine.
- The simulator window emulates the target platform's screen
  (`DEKI_SCREEN_WIDTH` x `DEKI_SCREEN_HEIGHT`) at a whole-number
  `windowScale`; `windowWidth` and `windowHeight` are gone. The display reports
  the screen's size, which is what the engine sizes the framebuffer to.
- Key ids come from deki-input's `Keys.h` instead of a copy of the numbers
  here. Tab and Delete are reported now; they used to be dropped.

## 0.16.0

### Fixed
- The input setup compiles in a runtime build. It calls into deki-input, whose
  namespace shares its name with its class, so a bare `DekiInput::` finds the
  namespace and not the class that owns `SetInput`. This code sits behind
  `#if !defined(DEKI_EDITOR)`, so the editor-side package compile check never
  saw it and only a simulator or firmware build did.

### Changed
- **Moved into the `DekiSdl3` namespace.** Every component was declared at global
  scope, which made its identity a bare class name — the name a scene file
  stores and the name the registry keys on — so two packages defining one name
  collided there with nothing to tell them apart. Each component carries
  `DEKI_FORMER_NAME` with the name it was saved under before, so existing
  scenes load unchanged and are written back qualified on the next save.
  Code naming these types needs the namespace: `using namespace DekiSdl3;` or a
  qualified name.
- Enum properties are stored by name rather than by number, so appending to an
  enum or reordering one no longer changes what a saved scene means. Files
  written before this still read.
- `minEngine` 0.16.0. Reflection ABI 17: the package must be rebuilt.

## 0.15.0

### Changed
- `Present` and `PresentRegions` take a `Deki::ColorFormat` instead of an
  `int`.
- Allocates through the engine with an explicit region instead of `new[]` and
  `malloc`.
