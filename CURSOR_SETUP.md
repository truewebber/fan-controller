# Cursor IDE Setup Notes

## PlatformIO IntelliSense Fix

**Affected versions:**
- Cursor `2.6.x` (tested on `2.6.19`, arm64)
- `davidgomes.platformio-ide-cursor` `0.0.1`
- `anysphere.cpptools` `2.0.x`

### Problem

The PlatformIO extension for Cursor (`davidgomes.platformio-ide-cursor`) crashes
silently on startup. It looks up its own metadata using the VS Code extension ID
`platformio.platformio-ide`, but in Cursor the actual ID is
`DavidGomes.platformio-ide-cursor`. The lookup returns `undefined`, throws
immediately, and the extension never finishes initializing.

Symptoms:
- PlatformIO sidebar stuck on "Initializing PlatformIO Core…"
- `anysphere.cpptools` reports `intelliSenseEngine is disabled`
- No IntelliSense for `Arduino.h`, AVR registers, library headers

### Fix

Patch one string in the extension's built JavaScript file:

```bash
sed -i '' \
  's/platformio\.platformio-ide/DavidGomes.platformio-ide-cursor/g' \
  ~/.cursor/extensions/davidgomes.platformio-ide-cursor-0.0.1-universal/dist/extension.js
```

Then reload the window: `Cmd+Shift+P` → **Reload Window**.

**Note:** the patch is overwritten if the extension updates. Re-apply after each update.

### IntelliSense setup

`anysphere.cpptools` uses clangd internally and reads `compile_commands.json`.
Regenerate it after adding libraries or changing `platformio.ini`:

```bash
pio run --target compiledb
```

The file is in `.gitignore` — regenerate it locally after cloning or when changing dependencies.

### References

- [Why PlatformIO IDE Gets Stuck on "Initializing PlatformIO Core" in Cursor](https://medium.com/@dasunnimantha777/why-platformio-ide-gets-stuck-on-initializing-platformio-core-in-cursor-and-how-to-fix-it-in-2-a6d4bd1c9955)
