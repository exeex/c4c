Status: Active
Source Idea Path: ideas/open/730_post_legacy_bir_shell_bootstrap.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Migrate the import spine and CFG publication

# Current Packet

## Just Finished

- Step 5 active-graph cleanup removed the legacy RISC-V `c4c-as` executable
  from the root build and removed every direct `c4c-as` consumer registered by
  backend tests: the assembler parse suite, objdump extraction suite, RV64
  roundtrip contract, and RV64 c-testsuite roundtrip scan target.
- The assembler and roundtrip sources remain untouched as legacy reference
  material; no legacy, prealloc, or MIR source was restored to active
  compilation.

## Suggested Next

- Quarantine the remaining prealloc/MIR/RV64 backend test registrations from
  the active CTest/build graph, retaining only tests for the new LIR-to-BIR and
  BIR-to-MIR interface boundaries as those interface tests become available.

## Watchouts

- `ctest -N` still enumerates many backend tests whose identities and expected
  behavior belong to the quarantined prealloc/MIR/RV64 implementation. They do
  not depend on `c4c-as`, so removing them was outside this packet, but they are
  the next active-test-graph cleanup boundary.
- The delegated proof did not include a full default build, so this packet does
  not claim that no later test-binary compile blocker exists.

## Proof

- `cmake --preset default` passed.
- `cmake --build --preset default -j 2 --target c4c_backend c4cll c4c-objdump`
  passed.
- `ctest --test-dir build -N` plus generated `CTestTestfile.cmake` inspection
  confirmed no registered test identity or command depends on `c4c-as` or the
  removed RV64 roundtrip registrations.
- `cmake --build --preset default --target help | rg
  '(^|/)c4c-as($|:)'` produced no match, confirming the target is absent.
- Complete delegated proof output is preserved in `test_after.log`.
