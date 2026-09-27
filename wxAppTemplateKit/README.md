# wxAppTemplate kit

A starting point for Windows desktop applications in C++20 with **wxWidgets**, **SQLite** and
**serial ports** - the reusable foundations of a real lab-control application (logging, settings,
translations, themes, versioning, serial transactions, database), stripped of everything
project-specific, plus a script that turns it into a new, named project with the builds you want.

Copyright (C) 2026 Fation Coga - [GNU LGPL v3.0 or later](LICENSE.md).

## Quick start

1. Install the dependencies and set `WXWIN`, `VC_SQLITE`, `VC_WJWWOOD_SERIAL` - see
   [DEPENDENCIES.md](DEPENDENCIES.md).
2. Double-click **`NewProject.bat`** and answer the questions:
   - application name (e.g. `Reactor Control`) - window title, exe name, data folder;
   - project identifier (e.g. `ReactorControl`) - `.sln` / `.vcxproj` name;
   - destination folder;
   - builds: any of the 8 below (`x64`, `win32`, `static`, `dll`, `debug`, `release`, `all`, or numbers);
   - copyright holder; optionally a git repository with a first commit.
3. Open the generated `.sln` and build. The exe is in `Builds\<Platform>\<Configuration>\`.

Without questions: `powershell -ExecutionPolicy Bypass -File NewProject.ps1 -Name "Reactor Control"
-Builds "x64,win32" -Destination C:\dev\ReactorControl -Yes`.

The `skeleton\` folder is itself a complete project (`TemplateApp.sln`) that builds as it is.

Checked before release: builds with no warnings (`-Wall -Wextra`, wxWidgets 3.2, Linux); every
identifier checked against the macros of the Windows SDK headers (MinGW); the parts that include
`windows.h` cross-compiled for Windows; a self-test of the `Utils` helpers (checksums and SHA-256
against their published check values, hex/bit/byte-order conversions, time packing, file helpers)
passes; a self-test of the serial terminal engine passes; the terminal was run against a virtual
serial port pair (auto replies, counters, checksums, periodic messages, patterns split over reads);
Doxygen reports no warnings.

## Builds

| # | Platform | Configuration | wxWidgets | Output |
|---|---|---|---|---|
| 1 | x64 | Debug | static libs | `Builds\x64\Debug` |
| 2 | x64 | Release | static libs | `Builds\x64\Release` |
| 3 | x64 | Debug_DLL | DLLs | `Builds\x64\Debug_DLL` |
| 4 | x64 | Release_DLL | DLLs | `Builds\x64\Release_DLL` |
| 5-8 | Win32 | same four | | `Builds\Win32\...` |

"static" links wxWidgets into the exe; "DLL" uses the wxWidgets DLLs (copied next to the exe by the
build). Both use the DLL C runtime by default - see DEPENDENCIES.md, "C runtime", for a fully static exe.

Every build:
- increments a **build number** (`BuildCounter.txt`) and records the **git commit** and **build date**
  (`include\GeneratedBuildInfo.h`, both gitignored);
- writes `<App name>.exe` (always the same name - the debugger's target) **and a versioned copy**
  `<App name> v0.1.0-rc.1b12.exe` (`D` appended for Debug). Older versioned copies are kept;
- copies the language files into `<App name> data\` and, for DLL builds, the wxWidgets DLLs.

## What is inside

| File | What it gives you |
|---|---|
| `AppInfo.h` | the application name, data folder name, copyright - in one place |
| `Version.h`, `BuildInfo.*` | semantic version + history; build number, git commit, build date; window title `Name (v0.1.0-rc.1b12, 2026-09-26 14:03)` |
| `DataDir.*` | every file lives in `<exe folder>\<App name> data\` (created on demand) - one folder to back up |
| `Log.*` | thread-safe log to file + GUI; levels None...Trace; the GUI drains a bounded queue on a timer (never flooded, nothing lost at startup) |
| `AppSettings.*` | `key=value` settings, merge-safe and crash-safe writes (temp file + rename) |
| `I18n.*`, `UVT.h` | run-time translations from `language-<code>.xml`; missing or empty entries fall back to English; `UVT.h` holds every visible text |
| `Theme.*` | light/dark at startup; live "Use now" switch that also themes windows opened later |
| `SerialConfig.*` | port settings, text persistence, port discovery, timeout floor (a 0/0 timeout blocks forever on Windows) |
| `SerialLink.*` | thread-safe request/reply on one port: timed lock, double input flush for RS-485, read until the frame is complete, abortable |
| `SerialWorker.*` | background request/reply transaction queue; results delivered on the GUI thread |
| `SerialMonitor.*` | terminal-style port: always listening, send queue, auto replies answered on its own thread; traffic delivered in batches on the GUI thread |
| `SerialData.*`, `SerialMessage.*` | Hex / ASCII / Mixed notation, console display (`[0D=CR]`), counters, checksums, message templates, pattern matching - no GUI, self-tested |
| `DataEntry.*`, `AsciiTableDialog.*`, `MessageDialogs.*`, `AutomationDialog.*` | the serial-terminal GUI pieces (see "Serial terminal" below) |
| `Database.*` | SQLite RAII wrapper: statements, transactions, WAL mode |
| `ThreadUtils.h` | `AliveGuard`, `CallLater` (delayed GUI call that is skipped if its window is gone), `WorkerQueue` (background task queue with a GUI-thread continuation) |
| `StatusLed.*` | coloured status indicator with tooltip, label and click / double-click / right-click events |
| `SerialConfigDialog.*` | port / baud / format / timeouts dialog |
| `MainWindow.*`, `App.*` | skeleton window: a small serial terminal (below), live log, event database, theme / language / log level with Restart now · Use now · Save for next start · Cancel, Version Info, About |

## Serial terminal

The skeleton window is a working serial terminal - a base to build a device tool on:

- **Always listening.** Once the port is open, everything received is printed in the serial console,
  one line per message (bytes closer together than the port's inter-byte timeout form one message).
  Printable ASCII is shown as it is, control characters by name (`[0D=CR]`, `[09=TAB]`, `[01=SOH]`),
  other bytes by value (`[C1]`); the hex bytes can be shown too. Sent, auto-reply and periodic
  messages are listed in their own colour, in the order they crossed the line.
- **Three notations** (radio buttons) wherever bytes are written:
  - *Hex* - only hex digits can be typed; the space between bytes is added automatically (pasted
    `0x4A,0x5B` becomes `4A 5B`);
  - *ASCII* - printable ASCII characters only;
  - *Mixed* - text, with `//0x` + two hex digits for any byte: `OK//0x0D//0x0A` (one `//0x` per byte).
  Switching notation converts what is written. The hint under the box shows the byte count or,
  if the text is wrong, the position and the reason.
- **Insert character...** opens the table of the 256 byte values (decimal, hex, character, name,
  description). Double-click adds the character and closes; right-click adds it and keeps the table
  open for more.
- **Auto replies** (Serial > Auto Replies...): when a pattern is received - even split over several
  reads - reply with a message, optionally after a delay. Any number of rules, each enabled or not.
- **Periodic messages** (Serial > Periodic Messages...): messages sent at their own interval.
- Replies and periodic messages can carry:
  - **incrementing values** - up to 64 bits (8 bytes), written over the message from a chosen byte
    index (the dialog says which bytes they occupy), as a binary number (big- or little-endian;
    big-endian by default) or as ASCII decimal / hex digits; start, end and step (negative to count
    down); advancing a number of steps per message sent or per second - fractions allowed (0.1 per
    second = one step every 10 s);
  - **a checksum** - SUM-8/16, XOR-8, LRC-8, CRC-8, CRC-8/MAXIM, CRC-16 (MODBUS, ARC, CCITT-FALSE,
    XMODEM), CRC-32, CRC-32C; inserted at a chosen index (default: at the end); calculated on all
    bytes or all except some indexes (`0, 5-7`); all its bytes or only the lower / upper part; in
    either byte order (the usual one of the algorithm is proposed, e.g. little-endian for Modbus).
  A live preview shows the first messages that will be sent.
- Everything (rules, send box, console options) is kept in the settings file.

`tests/SerialDataSelfTest.cpp` checks the engine (notations, display, counters incl. 64-bit
wrap-around, every checksum against its published check value, message building, the settings
text form, pattern matching across reads).

## General-purpose helpers (`Utils.h`)

The successor of the old `pUtl` class, split by topic into small namespaces - include `Utils.h`
for all of them or just the header you need. Every function is documented (Doxygen).

| Namespace | Header | Highlights |
|---|---|---|
| `Utils::Time` | `TimeUtils.h` | monotonic ms/µs clocks, epoch ms, `nowString()` "2026-09-26 14:03:01.123", `fileNameStamp()`, `durationString()` "1d 02:03:04.005", packed timestamps `YYYYMMDDhhmmssmmm` in a `uint64_t` (pack/unpack), `Stopwatch`, `sleepMs/Us` |
| `Utils::Hex` | `HexUtils.h` | `toHex()` for `uint8_t`...`uint64_t` (width follows the type, optional `0x`), `toBinary()` with digit grouping, `bytesToString()`, `wordsToString()`, `dump()` (offset + hex + ASCII), `parseBytes("55 01 0A")`, `parseNumber("0x1F" / "0b101" / "31")` |
| `Utils::Bits` | `HexUtils.h` | `isSet/set/clear/toggle/assign`, `mask`, `extract/insert` bit fields, `countOnes`, `lowest/highestSetBit`, `reverse`, `byteSwap`, `readBE/readLE`, `appendBE/appendLE`, float/double <-> IEEE-754 bit patterns |
| `Utils::Checksum` | `HexUtils.h` | `xor8` (BCC), `sum8`, `sum16`, `twosComplement8` (LRC), `crc8`, `crc8Maxim`, `crc16Modbus`, `crc16Arc`, `crc16CcittFalse`, `crc16Xmodem`, `crc32`, `crc32c`, and a generic `crc()` for any CRC up to 64 bits |
| `Utils::Str` | `TextUtils.h` | `trim`, `split`, `join`, `toLower/Upper`, `equalsIgnoreCase`, `replaceAll`, `ellipsize`, locale-independent `toInt/toDouble` |
| `Utils::Files` | `TextUtils.h` | `ensureDirectory`, `sanitizeFileName` (Windows-safe, strips `\\.\` device prefixes), `uniquePath` (never overwrites), `timestampedPath`, `readText`, `writeTextAtomic`, `appendLine`, `size`, `humanSize` |
| `Utils::Math` | `MathUtils.h` | `mapRange` (value -> pixel, ADC count -> volts), `roundTo`, `nearlyEqual`, `percent`, `divideRoundUp` |
| `Utils::Hash` | `HashUtils.h` | portable SHA-256 (`sha256`, `sha256Hex`) - no OS crypto API |
| `Utils::Gui` | `GuiUtils.h` | `padText` (wxString-safe), named colour palette + `colourByIndex`, `setInteractiveEnabled` (whole panels), `appendToConsole` (bounded console that keeps the user's scroll position), debug console window (Windows) |

## Documentation

Every file, class, function and constant carries a Doxygen comment (`/*! \brief ... */`). Run
`doxygen` in the project folder (the `Doxyfile` is included) and open `docs/html/index.html`; the
template produces no Doxygen warnings - keep it that way when adding code.

## Extending it

- **A device protocol (master polling devices):** build request bytes, then submit a
  `SerialWorker::Job` with a `FrameComplete` that tells from the first bytes how long the reply is
  (length byte, end marker, ...). Use `SerialWorker` *or* `SerialMonitor` on a port, not both (a
  transaction flushes the input the monitor is listening to).
- **A device simulator / tester:** use `SerialMonitor` with `AutoReplyRule`s built in code, or
  let the user define them in the dialogs.
- **A new visible text:** add it to `UVT.h`, show it with `tr(UVT::MY_TEXT)`, add the translation to
  each `languages\language-<code>.xml`.
- **A new language:** copy `language-it.xml` to `language-<code>.xml`, set `language` and `name` in its
  first element, translate the values. No rebuild needed.
- **A release:** update `Version.h` (numbers + history entry), commit, tag, build Release, hand out the
  versioned exe.

## Keeping translated text intact

Keep user-visible text in `wxString`. `wxString::ToStdString()` converts through the C locale and
returns an **empty** string for non-Latin scripts (Arabic, Russian, Chinese, ...). When a
`std::string` is unavoidable (files, SQLite), use `utf8_string()` and read back with
`wxString::FromUTF8()`.
