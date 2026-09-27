# wxAppTemplate kit - notes for Claude

`wxAppTemplateKit/`: skeleton wxWidgets 3.3 + SQLite + serial application for Visual Studio and the
`NewProject` generator. Projects made from it (e.g. SerialPM, Reactor Control/graph2d) follow the same rules.

## House rules

- **Dark mode for small tools.** A program without its own theme setting (tools, generators, small
  utilities) always starts dark: `wxApp::SetAppearance(Appearance::Dark)` in `OnInit()` before the
  first window (wxWidgets 3.3+), plus dark colours applied to every top-level window when first shown
  (`FilterEvent`, wxEVT_SHOW) - not to buttons, check boxes, choices and date pickers (the system
  draws them dark; forced colours hide their disabled look). Status colours readable on dark.
  Full applications keep a Light/Dark setting (`Theme.h`).
- **Application icon**: `art/app.ico` as `appicon` (the first icon in the `.rc`: Explorer, taskbar,
  Alt-Tab; never `wxICON_AAA` - wx.rc already uses that name) and set on every top-level window/dialog in `FilterEvent` (`wxIconBundle("appicon", nullptr)`).
- **Libraries** (DEPENDENCIES.md): `VC_SQLITE` / `VC_WJWWOOD_SERIAL` point at folders holding
  `include\` and `<x64|x86>\<Debug|Release>\lib|dll\` (built by SQLite3_builder and the CogaF/serial
  fork, without `/GL`). Static configurations link `lib`, `_DLL` configurations link `dll` and copy
  every needed DLL next to the exe. Never hardcode a platform.
- No zip of the kit in the repository (GitHub's Download ZIP is always current).
- Push directly to `main` once a change is approved.
