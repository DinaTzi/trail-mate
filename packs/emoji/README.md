# Emoji Extension

This bundle provides optional emoji support for Trail Mate chat content.

It is a content/input extension, not a locale:

- font pack: `emoji-core`
- IME pack: `emoji-picker`
- package type: `content-bundle`

The runtime uses `emoji-core` as a content supplement when installed and when
the active memory profile allows supplement fonts. The picker uses the generic
`builtin-candidate-picker` backend and reads its candidates from pack payload.

## Source Font

`emoji-core` is generated from Noto Emoji Monochrome:

- file: `tools/fonts/NotoEmoji-Regular.ttf`
- upstream: `https://github.com/zjaco13/Noto-Emoji-Monochrome`
- original project: `https://github.com/googlefonts/noto-emoji`
- license: SIL Open Font License 1.1

The generated `font.bin` is intentionally not committed. Pages/package builds
regenerate it from `charset.txt` and `build.ini`.

## Build

```bash
python scripts/build_pack_repository.py --pack-root packs --site-root site
```

The resulting archive installs runtime payload under:

```text
/trailmate/packs/fonts/emoji-core/manifest.ini
/trailmate/packs/fonts/emoji-core/ranges.txt
/trailmate/packs/fonts/emoji-core/font.bin
/trailmate/packs/ime/emoji-picker/manifest.ini
```
