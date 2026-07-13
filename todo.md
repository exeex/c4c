Status: Active
Source Idea Path: ideas/open/730_post_legacy_bir_shell_bootstrap.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Restore CMake generation without legacy sources

# Current Packet

## Just Finished

- Step 1 removed the obsolete prepared-BIR, prealloc, route, semantic-BIR, and
  `backend_lir_to_bir_notes` target/registration blocks from
  `tests/backend/bir/CMakeLists.txt`.
- CMake configuration and generation now succeed without any
  `src/backend/legacy/` entry in `build/compile_commands.json`.

## Suggested Next

- Add the new BIR shell header at `src/backend/bir/bir.hpp` and reconcile the
  direct `backend.hpp` consumer; the first compiler seam is
  `backend.hpp:3:10: fatal error: bir/bir.hpp: No such file or directory`.

## Watchouts

- Never compile or transplant `src/backend/legacy` code.
- Bootstrap only explicit empty BIR/MIR behavior; do not imply target codegen.
- Retain tests only for direct LIR-to-new-BIR and new-BIR-to-MIR contracts.
- The five remaining BIR view-contract targets are the next test-retention
  decision; this packet did not edit test sources or widen beyond the obsolete
  target blocks delegated for Step 1.

## Proof

- `cmake --build --preset default -j 2` — CMake generation succeeded, then the
  compile stopped at missing `bir/bir.hpp` in `src/backend/backend.hpp` (also
  reached through `src/codegen/llvm/llvm_codegen.cpp`).
- `rg -n '/src/backend/legacy/' build/compile_commands.json` — no matches.
- No `test_after.log` was written because root test logs were outside this
  packet's owned files; proof output is available in the executor handoff.
