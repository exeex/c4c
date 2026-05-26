Status: Active
Source Idea Path: ideas/open/38_aarch64_module_compatibility_foldback_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Fold Or Delete Compatibility Projection Construction

# Current Packet

## Just Finished

Step 2 - Fold Or Delete Compatibility Projection Construction completed the
mechanical module-owner fold-back packet:

- Moved `derive_compatibility_function_records` and
  `derive_compatibility_projection` into the anonymous namespace in
  `module_compile.cpp`.
- Removed the `compatibility_projection.hpp` include from `module_compile.cpp`.
- Removed `compatibility_projection.cpp` from `src/backend/CMakeLists.txt`.
- Deleted obsolete `compatibility_projection.cpp` and
  `compatibility_projection.hpp`.
- Updated `backend_aarch64_signature_metadata_test.cpp` only for deleted-file
  metadata fallout.
- Kept `built_module.functions` and `built_module.compatibility` populated
  exactly as before, preserving legacy projection behavior for current readers.

## Suggested Next

Next mechanical packet: clean stale live compatibility ownership references and
validate the module compatibility fold-back route. Search live source/build/test
paths for `compatibility_projection`; preserve deliberate historical contract
wording unless the supervisor explicitly owns a docs/comment cleanup.

## Watchouts

Keep this route mechanical. Do not make `FunctionRecord::machine_nodes` a
terminal assembly authority, route new behavior through compatibility
projection records, rewrite final assembly emission, alter MIR stream
ownership, or mix in calls/dispatch/memory/comparison/prologue cleanup. The
module-private helper names still contain `compatibility` by design; do not
interpret those names as stale standalone-owner references.

## Proof

Passed:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_signature_metadata|backend_aarch64_module_skeleton_contract|backend_aarch64_function_traversal|backend_aarch64_return_lowering|backend_aarch64_branch_control_lowering|backend_aarch64_prepared_scalar_alu_records)$' | tee test_after.log`

`test_after.log` is the proof log.
