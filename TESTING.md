# Testing Guide

This repository has host-side unit tests for utility and parser logic, plus
runtime validation paths through Ares.

## Host-Side Unit Tests

Primary reference: `tests/README.md`

Current suites:
- `test_fs` for filesystem utilities and OOB-hardening edge cases.
- `test_path` for menu path manipulation helpers.
- `test_ini_parser` for INI parsing, typing, and save/load behavior.

Run in devcontainer image:

```sh
docker run --rm -v "${PWD}:/workspaces/N64FlashcartMenu" -w /workspaces/N64FlashcartMenu/tests \
  n64flashcartmenu-sc64deployer bash -lc "make -B test"
```

Helper scripts:
- Windows: `tests/run_tests.bat`
- POSIX shell: `tests/run_tests.sh`

## Runtime / Emulator Validation

Build and run targets are in the root `Makefile`:
- `make run-ares`
- `make run-ares-debug`
- `make gdb`

Optional smoke validation for Ares debug server:
- `tests/gdb_smoke.ps1`

## Notes

- Generated host test binaries are ignored and should not be committed.
- For contributor workflow details, also see `docs/99_developer_guide.md`.
