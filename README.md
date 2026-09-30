# dragoman-kcm

The **Offline Translation** page of KDE System Settings (under Language
& Time), for [Dragomand](https://github.com/VuteTech/dragomand)
(website: [dragomand.l10n-bg.dev](https://dragomand.l10n-bg.dev)), the
per-user daemon that runs Mozilla's Firefox translation models locally,
so no text leaves the computer.

- **Memory and performance**: the memory budget for loaded language
  models, how many recently used pairs stay loaded when idle and for how
  long, and how soon the daemon leaves memory once nothing is loaded.
- **Network**: allow downloads and update checks at all, and opt into
  Mozilla's pre-release (nightly) models.
- **Language models**: the installed pairs with version, size, origin
  (downloaded or from a system package), a quality label derived from
  Mozilla's COMET scores, and an Experimental mark for models that are
  not yet released. Downloaded copies can be removed; the provider can
  be checked for updates, and every update installed with progress. When
  [Krakoman](https://github.com/VuteTech/krakoman) is installed, a button
  opens it to install more languages.
- **Status**: the daemon's version, memory in use and loaded pairs. It is
  fetched on demand only, because polling would keep an idle daemon from
  exiting.

The settings live in the daemon, not in a file of this module: the page
reads them with `GetConfig`, and Apply sends only the changed keys with
`SetConfig`, which the daemon checks, saves to its `config.toml` and
applies at once. A change made elsewhere (for example with
`dragomanctl config`) shows up immediately; values edited on the page
but not yet applied are kept. A daemon older than 0.2 has no
`GetConfig`; the page then says so and disables the settings, while the
language models still work.

Try it without System Settings:

```sh
kcmshell6 kcm_dragomand
```

## Build from source

Needs CMake 3.24, extra-cmake-modules,
[libdragoman-qt](https://github.com/VuteTech/libdragoman-qt), KF6
(CoreAddons, I18n, KCMUtils, KIO, Service) 6.13 or newer, Qt 6.8 or newer
(Core, DBus, Gui, Qml, Quick), and at run time Kirigami and Kirigami
Addons, plus Dragomand itself (which also installs the icon):

```sh
cmake -S . -B build -G Ninja   # add -DDragomanQt_DIR=<libdragoman-qt build>/buildtree for an uninstalled library
cmake --build build
ctest --test-dir build
sudo cmake --install build
```

To load an uninstalled build, point Qt at its plugin directory:

```sh
QT_PLUGIN_PATH=$PWD/build/bin:$QT_PLUGIN_PATH kcmshell6 kcm_dragomand
```

The tests run the settings, the model list and the status against
libdragoman-qt's fake daemon on a private `dbus-daemon`; they never touch
the session bus.

## Layout

```
src/*.{h,cpp}           the module and the logic behind the page
src/ui/main.qml         the page (Kirigami Addons FormCard)
src/kcm_dragomand.json  plugin metadata: name, icon, System Settings category
autotests/              tests against the fake daemon
po/                     translations (scripts/update-translations.sh)
```

## Translations

`scripts/update-translations.sh` extracts the strings into
`po/kcm_dragomand.pot` and merges them into every
`po/<language>/kcm_dragomand.po`. The module's name and description are
translated in `src/kcm_dragomand.json`.

## License

GPL-3.0-or-later. The project follows the [REUSE](https://reuse.software)
specification: every file states its license, and the texts are in
`LICENSES/`.
