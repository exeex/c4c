Status: Active
Source Idea Path: ideas/open/730_post_legacy_bir_shell_bootstrap.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Migrate the import spine and CFG publication

# Current Packet

## Just Finished

- Step 5 collapsed `tests/backend/CMakeLists.txt` to the retained post-legacy
  interface graph: only `tests/backend/bir` is registered today.
- Removed every legacy MIR, preallocation, target, runtime, dump, object, and
  emission test registration from the active CMake graph without deleting the
  archived test sources.
- `c4c_backend_tests` now depends only on active targets exported by the BIR
  interface-test directory.

## Suggested Next

- Add a new BIR-to-MIR interface directory and shell test only after that new
  interface exists; then register it beside `bir` in the minimal test graph.

## Watchouts

- `tests/backend/mir` and old backend case/source artifacts intentionally remain
  in the tree as unregistered references; they must not be re-added wholesale.
- There is no new BIR-to-MIR interface target yet, so the retained regex selects
  only `backend_lir_to_bir_interface` today.

## Proof

- `cmake --preset default && cmake --build --preset default -j 2 && ctest
  --test-dir build --output-on-failure -R
  '^backend_(lir_to_bir|bir_to_mir)'` passed; the retained LIR-to-BIR interface
  test is green and the full default build succeeds.
- `git diff --check` passed.
- `build/compile_commands.json` contains no translation unit under
  `src/backend/legacy`, `src/backend/prealloc`, or `src/backend/mir`.
- Complete delegated proof output is preserved in `test_after.log`.
