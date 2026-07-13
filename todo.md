Status: Active
Source Idea Path: ideas/open/730_post_legacy_bir_shell_bootstrap.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Freeze the bounded schema and API checkpoint

# Current Packet

## Just Finished

- Step 1 completed at `793eeeb90`: removed obsolete prepared-BIR, prealloc,
  route, semantic-BIR, and `backend_lir_to_bir_notes` target/registration
  blocks from `tests/backend/bir/CMakeLists.txt`.
- CMake configuration and generation now succeed without any
  `src/backend/legacy/` entry in `build/compile_commands.json`.
- The default build reaches the first production seam:
  `src/backend/backend.hpp` includes missing `bir/bir.hpp`.

## Suggested Next

- Execute Step 2: derive and review the bounded core file/type/API checkpoint
  from the 715 contract and blueprint before implementing `bir.hpp` or broad
  core infrastructure.

## Watchouts

- `bir.hpp` must be a facade over a real core, not a legacy copy or empty fake.
- Preserve owner/generation IDs, separate storage/order, builder-only mutation,
  RawBir publication, and terminator-only CFG authority.
- Do not implement the full P0--P13 roadmap without a concrete migration need.
- Not-yet-migrated LIR forms must reject safely; never fall back to legacy or
  silently disappear.

## Proof

- `cmake --build --preset default -j 2` — CMake generation succeeded, then
  compile stopped at missing `bir/bir.hpp` in `src/backend/backend.hpp` (also
  reached through `src/codegen/llvm/llvm_codegen.cpp`).
- `rg -n '/src/backend/legacy/' build/compile_commands.json` — no matches.
- No new proof run; this update changes lifecycle intent and runbook only.
