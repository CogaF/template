# Template App

Created from the wxAppTemplate kit.

- Build: open `TemplateApp.sln` in Visual Studio (dependencies: [DEPENDENCIES.md](DEPENDENCIES.md)).
- Output: `Builds\<Platform>\<Configuration>\Template App.exe` plus a versioned copy.
- Data (settings, log, databases, languages): `Template App data\` next to the exe.
- Version and history: `include\Version.h`.
- Name, author, contact and links (About window): `include\AppInfo.h`.
- Icon: `art\app.ico` - redraw it with `art\make-icon.ps1 -Top WORD -Bottom WORD`.

Copyright (C) 2026 Fation Coga - [GNU LGPL v3.0 or later](LICENSE.md).
