Status: Active
Source Idea Path: ideas/open/688_initializer_lowering_bridge_isolation.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Verify Known Global Address Import Boundaries

# Current Packet

## Just Finished

Step 5 verified the known global-address import/publication boundaries without
source changes.

Boundary findings:
- `resolve_known_global_address()` remains the only known global-address
  publisher; it writes `GlobalInfo::known_global_address` in
  `src/backend/bir/lir_to_bir/globals.cpp`.
- The module adapter call order still resolves aggregate pointer initializer
  offsets, string-pointer target ids, and resolved pointer value ids before
  publishing known global-address aliases in
  `src/backend/bir/lir_to_bir/module.cpp`.
- `is_known_function_global_address()` and the fenced raw-symbol bridge still
  have existing memory-lowering consumers, so removing or renaming them from
  `lowering.hpp` would cross into unowned memory files rather than tighten this
  packet's owned boundary.

No helper signature/name tightening was useful inside the owned files. No
emitted BIR facts, prepared object data, relocation spelling, target behavior,
diagnostics, expectations, unsupported markers, tests, allowlists, or harness
policy were changed.

## Suggested Next

Next coherent packet: supervisor should decide whether Step 5 is complete as a
verified no-op or whether a broader packet should include the memory-lowering
consumers before any public helper signature/name tightening is attempted.

## Watchouts

- `src/backend/bir/lir_to_bir/memory/provenance.cpp` and
  `src/backend/bir/lir_to_bir/memory/local_slots.cpp` consume
  `is_known_function_global_address()` from `lowering.hpp`; they were inspected
  only to establish the boundary and were not edited.
- `GlobalInfo::known_global_address` is still consumed by memory provenance
  lowering after the module adapter publishes it. Tightening that data shape is
  broader than this packet's owned files.
- Avoid turning future boundary cleanup into classification-only churn; a
  useful follow-up should either remove an actual cross-file dependency or keep
  behavior bit-for-bit identical.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_|string_authority_guard$)'`

Result: passed. Build completed and the selected backend plus
`string_authority_guard` subset passed with 0 failures. Fresh proof output is
preserved in `test_after.log`.
