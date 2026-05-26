Status: Active
Source Idea Path: ideas/open/36_aarch64_dispatch_publication_foldback_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Fold Dispatch Diagnostics Into Dispatch Owner

# Current Packet

## Just Finished

Step 2 - Fold Dispatch Diagnostics Into Dispatch Owner completed the
mechanical diagnostics fold-back packet:

- Moved dispatch-only helpers `classify_instruction`,
  `append_block_diagnostic`, `unsupported_terminator_message`, and
  `append_unsupported_instruction_diagnostic` into the anonymous namespace in
  `dispatch.cpp`.
- Moved the calls-used `append_call_diagnostic` helper into `calls.cpp`.
- Removed `dispatch_diagnostics.cpp` from `src/backend/CMakeLists.txt`.
- Deleted obsolete `dispatch_diagnostics.cpp` and
  `dispatch_diagnostics.hpp`.
- Kept diagnostic strings, diagnostic kinds, fail-closed behavior,
  ABI/prepare behavior, and test expectations unchanged.

## Suggested Next

Next mechanical fold-back packet: handle `dispatch_publication_common.hpp`
through the publication owner API. Start by deciding which declarations can be
namespace-local to `dispatch_publication.cpp` and which broad AArch64 consumers
still require declarations in `dispatch_publication.hpp`.

## Watchouts

This idea is mechanical fold-back only. Do not change prepared
edge-publication authority, value-home lookup semantics, diagnostic meaning, or
unsupported-path contracts. `dispatch_publication_common.hpp` is not currently
publication-private as a header; most of its declarations still need an owner
API visible to several AArch64 files.

## Proof

Passed:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner|backend_aarch64_prepared_handoff_gate|backend_aarch64_prepared_register_conversion)$' | tee test_after.log`

`test_after.log` is the proof log.
