# installer/deps - the bundled installer kit

`deps/` makes this project self-contained: it holds a copy of the shared Windows installer kit
(`win-installer-kit`: `installer_common.bat`, `installer_common_gui.bat`, `shared_setup.iss`,
`shared_install_wizard.iss`, `gui/build_selector.ps1`). With it, `installer\build_installer.bat`
works on a PC that has nothing else installed except Inno Setup and the compilers.

## The subfolder name

In the kit (`wxAppTemplateKit/skeleton`) the subfolder is called `__INSTALLER_KIT_FOLDER__`.
`NewProject.ps1` renames it when the project is created, from what you answered:

| Your answer | Subfolder name | Meaning |
|---|---|---|
| You already have an installer-kit environment variable, or you let the generator set it now (`setx`) | `<ENV_VAR_NAME>` (e.g. `MY_INSTALLER_KIT`) | nothing left to do |
| You did not set it | `set_this_folder_as_env_var_<ENV_VAR_NAME>` | a visible reminder: set the variable `<ENV_VAR_NAME>` to the full path of this folder (or of any other copy of the kit) |

To set it yourself later: `setx <ENV_VAR_NAME> "C:\full\path\to\installer\deps\set_this_folder_as_env_var_<ENV_VAR_NAME>"`
(open a new terminal afterwards). You may then rename the folder to `<ENV_VAR_NAME>` (and change the
fallback path in `build_installer.bat`, `build_installer_withGui.bat` and the `.iss`), or leave it as it is.

The chosen variable name is written into `installer\build_installer.bat`,
`installer\build_installer_withGui.bat` and `installer\<Project>.iss` in place of the template's
`__INSTALLER_ENV_VAR__`.

## Lookup order at build time

1. `%<ENV_VAR_NAME>%\installer_common.bat` (the `_gui` variant for `build_installer_withGui.bat`)
2. `installer\deps\<subfolder>\installer_common.bat` - the bundled copy

The `.iss` does the same for `shared_setup.iss` / `shared_install_wizard.iss`: `GetEnv("<ENV_VAR_NAME>")`
first, `deps\<subfolder>` when it is empty (a compiler message tells which one was used). So the project
always builds, even with the variable unset; the bundled copy just may be older than the shared one.

## Updating the bundled kit

The master copy is the `win-installer-kit` repository. To update, copy over this subfolder the files
`installer_common.bat`, `installer_common_gui.bat`, `shared_setup.iss`, `shared_install_wizard.iss`,
`gui\build_selector.ps1` and `README.md` of that repository (keep the subfolder name), then commit.
If you point the environment variable at a clone of that repository, you get updates with `git pull`
and the bundled copy is only the fallback.
