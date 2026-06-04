# Third-Party Libraries

## raylib 5.5

- Website: https://www.raylib.com/
- License: zlib/libpng
- Vendored headers and prebuilt libraries in `lib/raylib/`.
- No modifications.

## tinyfiledialogs 2.9.3

- Website: https://sourceforge.net/projects/tinyfiledialogs/
- License: zlib
- Single-file C library for native OS file dialogs. Vendored in
  `lib/tinyfiledialogs/`.
- Compiled as a separate object with `-D_POSIX_C_SOURCE=200809L` to avoid
  C23 strict mode implicit declaration errors for `popen`/`pclose`.

### Modifications

- `tinyfiledialogs.c`: `zenityPresent()` patched to always return 0.
  Zenity has GTK/Adwaita theme conflicts on some Linux desktops that cause
  the dialog to hang or render behind the application window. With this
  change the library falls through to kdialog or other available backends.
