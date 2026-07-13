Status: Active
Source Idea Path: ideas/open/730_post_legacy_bir_shell_bootstrap.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Migrate the import spine and CFG publication

# Current Packet

## Just Finished

- Step 5 replaced all registrations in `tests/backend/bir` with one durable
  direct `LirModule` to new `RawBir` interface test.
- The interface test proves void declaration import, void-return lowering,
  unconditional-branch CFG successor traversal, and structured rejection of
  an unsupported module-global semantic family.
- Removed the legacy BIR lowering include plus five obsolete backend-boundary
  functions and calls from `frontend_hir_tests.cpp`; no unsupported expectation
  or compiler behavior was weakened.

## Suggested Next

- Add the initial new BIR-to-MIR interface shell test, or quarantine remaining
  backend test registrations that still compile prealloc/MIR/RV64 legacy APIs.

## Watchouts

- The delegated target compiles the new interface test but does not rebuild
  `frontend_hir_tests`; its legacy-function deletion still needs the
  supervisor's broader build validation before acceptance.
- Other backend test directories remain outside this packet and may still
  register tests against quarantined prealloc/MIR/RV64 structures.

## Proof

- `cmake --preset default && cmake --build --preset default -j 2 --target
  backend_lir_to_bir_interface_test && ctest --test-dir build
  --output-on-failure -R '^backend_lir_to_bir_interface$'` passed.
- `git diff --check` passed.
- Complete delegated proof output is preserved in `test_after.log`.
