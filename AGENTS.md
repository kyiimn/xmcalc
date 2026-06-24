# AGENTS.md — xmcalc

## What This Is

**xmcalc** is a fork of the X.Org `xcalc` scientific calculator, ported from the Athena Widget Set (Xaw/Command+Form) to OpenMotif (Xm/PushB+Form+Label+ToggleB+Frame+TextField+RowColumn). The original Xaw version lives at <https://gitlab.freedesktop.org/xorg/app/xcalc>. This project is **not** a new application — it is a direct widget-set migration of a ~1987 codebase, preserving the original calculator engine (`math.c`, `actions.c`) almost unchanged while replacing the UI layer.

## Build System

- **Autotools** (autoconf/automake). After cloning from a tarball or fresh checkout, run `./autogen.sh` to regenerate the build system. If `configure` already exists, `./configure && make` suffices.
- The binary is `xmcalc` (not `xcalc`). The configure `--program-prefix` trick is NOT used — the name is hardcoded as `xmcalc` in `configure.ac`.
- Link flags: `-lXm -lXt -lX11 -lm`. The `configure.ac` tries `pkg-config --exists xm` first, then falls back to `motif`, then `AC_CHECK_LIB([Xm], …)`.
- `AM_CFLAGS` includes `-D_CONST_X_STRING` (Motif compatibility macro).
- App-defaults files install to `${datadir}/X11/app-defaults/` by default (`--with-appdefaultdir` to override).

## Source Layout

| File | Purpose |
|------|---------|
| `xmcalc.c` | Main entry, widget creation, button layout, translations, keyboard accelerators. The bulk of the Xaw→Xm migration lives here. |
| `xmcalc.h` | Shared header: key constants (`kRECIP`, `kSIN`, …), indicator constants, extern declarations. |
| `actions.c` | Xt action dispatch table — maps action names (`"sine"`, `"digit"`, etc.) to C functions. Unchanged from original except `_X_NORETURN` annotations. |
| `math.c` | Calculator engine: numeric entry, stack operations, trig, RPN/infix dual-mode. Essentially unmodified from original xcalc. |
| `app-defaults/XmCalc` | X resources for widget layout, labels, translations (button bindings + keyboard accelerators). **Critical**: Motif does NOT apply `labelString` or `translations` from resource files the same way Xaw did — the code now sets these programmatically. The resource file is kept as a fallback/reference. |
| `app-defaults/XmCalc-color` | Color theme (`#include "XmCalc"` + color overrides). |
| `man/xmcalc.man` | Man page (troff). Substituted at build time via `MAN_SUBSTS`. |

## Key Architecture Notes

### Dual Mode: TI-30 (infix) vs HP-10C (RPN)

- Controlled by `-rpn` flag or `XmCalc.rpn: on` resource. The global `int rpn` flag switches behavior.
- TI mode: 55 buttons in an 11×5 grid. HP mode: 39 buttons (buttons 21 & 22 are managed but invisible — `mappedWhenManaged=False` — to preserve XmForm attachment targets).
- Layout is done **programmatically** in `set_ti_attachments()` / `set_hp_attachments()` because Motif's String-to-Widget converter for `XmNleftWidget`/`XmNtopWidget` resources does not work at runtime. The app-defaults file has the same attachments as comments/fallback, but the code explicitly sets them via `XtSetArg/XtSetValues`.
- Button labels and translations are also set programmatically (`set_ti_labels`, `set_hp_labels`, `set_ti_translations`, `set_hp_translations`) for the same reason.

### Motif Migration Gotchas

- **No String-to-Widget converter**: Motif cannot resolve widget names in resource strings like `XmNleftWidget: button1`. This is why ALL form attachments, labels, and translations are set in C code.
- **No String-to-Bitmap converter**: The original xcalc used `XmCalc.IconPixmap` resource with a bitmap filename. This is removed; icon pixmaps must be set programmatically if needed.
- **`XmNmappedWhenManaged`**: Used for HP buttons 21/22 to keep them in XmForm layout while hiding them visually. Size must be re-applied after changing visibility properties because Motif recalculates preferred size.
- **ENTER button**: Uses a smaller font (`6x12`) and multiline label `"E\nN\nT\nE\nR"` with explicit `XmNheight` of 60px to fit the text vertically.
- **LCD display**: An `XmLabel` (not `XmTextField`) with `XmNalignment: XmALIGNMENT_END` and `XmNrecomputeSize: False`. The original used `XawAsciiText`; the Motif version uses `XmLabel` for read-only display.
- **Keyboard input**: Translations are registered on the LCD widget AND the toplevel shell (`XtOverrideTranslations` with `WM_PROTOCOLS` handler). `XtSetKeyboardFocus(calculator, LCD)` ensures key events reach the LCD.
- **Arm/Disarm in translations**: Button translations use `#override<Btn1Down>:Arm()\n<Btn1Up>:action() Disarm()` pattern. Without `Arm()`/`Disarm()`, Motif PushButton won't show visual press feedback.

### Calculator Engine (`math.c`)

- Dual-mode (infix/RPN) with shared state machine. The `rpn` global selects behavior.
- Stack: `numstack[STACKMAX]` (32 deep) for infix, 3-register stack for RPN (`numstack[0..2]` = X, Y, Z).
- Memory: 10 cells (`mem[XMCALC_MEMORY]`) in RPN mode, single cell in TI mode.
- `strlcpy` has an inline fallback when `HAVE_STRLCPY` is not defined.
- Non-IEEE builds use `setjmp/longjmp` via `XMCALC_PRE_OP` macro for FPE/SIGILL recovery. IEEE builds skip signal handlers.

## Developer Commands

```sh
# Build from fresh clone
./autogen.sh          # runs autoreconf + configure
make                  # builds xmcalc

# Build from existing configure
./configure && make

# Run
./xmcalc              # TI-30 mode (default)
./xmcalc -rpn         # HP-10C mode
./xmcalc -stipple     # stippled background (useful on mono displays)
./xmcalc -version     # print version
./xmcalc -help        # usage

# Install
sudo make install     # installs binary + app-defaults + man page

# Clean
make clean
make distclean        # also removes generated configure/Makefile
```

## Testing

There are **no automated tests**. Verification is manual: run the binary and exercise calculator functions in both TI and HP modes. Key things to check after changes:
- TI mode: all 55 buttons visible and functional, correct labels
- HP mode: 39 buttons visible, buttons 21/22 invisible, ENTER button tall with multiline label
- Keyboard accelerators work in both modes
- LCD display shows numbers right-aligned
- Mode indicators (M, INV, DEG/RAD/GRAD, P, HEX/DEC/OCT) toggle correctly
- WM_DELETE_WINDOW closes the application

## Commit Convention

Recent commits follow `type: description` format:
- `fix:` — bug fixes
- `feat:` — new features
- `ui:` — UI/layout changes
- `resources:` — resource file changes
- `rename:` — name changes (xcalc→xmcalc)

## References

- Original xcalc: <https://gitlab.freedesktop.org/xorg/app/xcalc>
- Bug reports: <https://gitlab.freedesktop.org/xorg/app/xmcalc/-/issues>
- X.org mailing list: <https://lists.x.org/mailman/listinfo/xorg>
- Patch submission: <https://www.x.org/wiki/Development/Documentation/SubmittingPatches>