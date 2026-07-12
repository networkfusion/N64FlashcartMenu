# Host-Side Unit Tests

This folder contains host-native tests that validate logic without requiring N64 runtime execution.

## Suites

- `test_fs`: Filesystem utility behavior and OOB-hardening checks.
- `test_path`: Menu path manipulation behavior (`src/menu/path.c`).
- `test_ini_parser`: INI parser behavior (`src/menu/ini_parser.c`), including quoted values and save/load round-trip.

## Run in Devcontainer Image

```sh
docker run --rm -v "${PWD}:/workspaces/N64FlashcartMenu" -w /workspaces/N64FlashcartMenu/tests \
  n64flashcartmenu-sc64deployer bash -lc "make -B test"
```

## Notes

- `tests/stubs/libdragon.h` is a tiny host-test shim providing `debugf` so `ini_parser.c` can be compiled outside libdragon runtime.
- Generated test binaries (`test_fs`, `test_path`, `test_ini_parser`) are not source files and should not be committed.
