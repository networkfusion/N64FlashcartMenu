# stdio/File I/O audit (first-party code)

## Scope
- Included: first-party files under `src/menu`, `src/utils`, `src/flashcart`, `src/boot`, and `src/main.c`.
- Excluded: third-party/vendor code under `src/libs` and `libdragon`.

## Inventory summary
- 149 stdio/seek calls (`fopen`, `fread`, `fseek`, `ftell`, `fclose`) in 17 first-party files.
- Hotspots are concentrated in menu runtime features and utility file helpers.

## Files using stdio/seek
- `src/menu/datel_codes.c`
- `src/menu/disk_info.c`
- `src/menu/id3_parser.c`
- `src/menu/ini_parser.c`
- `src/menu/mp3_player.c`
- `src/menu/png_decoder.c`
- `src/menu/rom_info.c`
- `src/menu/ui_components/background.c`
- `src/menu/usb_comm.c`
- `src/menu/views/cpak_dump_info.c`
- `src/menu/views/cpak_note_dump_info.c`
- `src/menu/views/cpakfs_manager.c`
- `src/menu/views/extract_file.c`
- `src/menu/views/text_viewer.c`
- `src/menu/zip_entry_count.c`
- `src/utils/cpakfs_utils.c`
- `src/utils/fs.c`

## Migration classification

### Keep FILE* (seek-heavy/streaming)
These modules depend on position-based streaming and should keep FILE* interfaces for now:
- `src/menu/mp3_player.c`
- `src/menu/id3_parser.c`
- `src/menu/zip_entry_count.c`
- `src/menu/views/cpak_dump_info.c`
- `src/menu/disk_info.c`

### Good candidates for wrapper migration first
These mostly do bounded open/read/write/close and can be standardized first:
- `src/menu/views/extract_file.c`
- `src/menu/usb_comm.c`
- `src/menu/views/text_viewer.c`
- `src/menu/datel_codes.c`
- `src/menu/png_decoder.c`
- `src/utils/cpakfs_utils.c`

### Mixed complexity (phase after wrappers exist)
- `src/menu/ini_parser.c`
- `src/menu/rom_info.c`
- `src/menu/ui_components/background.c`
- `src/menu/views/cpak_note_dump_info.c`
- `src/menu/views/cpakfs_manager.c`

## Recommended phased rollout
1. Add utility wrappers in `src/utils/fs.h` and `src/utils/fs.c` for common patterns:
   - read entire file
   - write entire file
   - safe size lookup + error propagation
2. Migrate low-risk call sites first (candidate group above).
3. Re-run docker build and runtime smoke checks.
4. Decide whether seek-heavy modules should remain FILE* or gain dedicated adapter APIs.

## Success criteria
- No behavior changes in ROM/menu flows.
- Less duplicated open/read/close error handling.
- Clearer boundary: simple file operations through shared helpers; advanced streaming remains explicit.
