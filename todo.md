Status: Active
Source Idea Path: ideas/open/36_aarch64_dispatch_publication_foldback_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Fold Publication-Common Declarations Into Publication Owner

# Current Packet

## Just Finished

Step 3 - Fold Publication-Common Declarations Into Publication Owner completed
the mechanical header ownership packet:

- Moved all declarations formerly exposed by
  `dispatch_publication_common.hpp` into `dispatch_publication.hpp`.
- Removed redundant `dispatch_publication_common.hpp` includes from AArch64
  codegen files that already included `dispatch_publication.hpp`.
- Replaced the remaining live include in
  `prepared_value_home_materialization.cpp` with `dispatch_publication.hpp`.
- Deleted obsolete `dispatch_publication_common.hpp`.
- Kept prepared publication, value-home lookup, edge-copy semantics,
  diagnostics, and test expectations unchanged.

## Suggested Next

Next mechanical packet: continue with the publication fold-back route by
inventorying or folding the remaining dispatch publication implementation
surface according to the active runbook, without changing publication
semantics.

## Watchouts

This idea is mechanical fold-back only. Do not change prepared
edge-publication authority, value-home lookup semantics, diagnostic meaning, or
unsupported-path contracts. The former common declarations remain broadly
consumed across AArch64 codegen and now live on the publication owner header.

## Proof

Passed:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_call_boundary_owner|backend_aarch64_prepared_handoff_gate|backend_aarch64_prepared_register_conversion)$' | tee test_after.log`

`test_after.log` is the proof log.
