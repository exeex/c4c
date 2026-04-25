Status: Active
Source Idea Path: ideas/open/113_backend_struct_decl_layout_table_dual_path.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Convert Aggregate Size And Addressing Consumers

# Current Packet

## Just Finished

Step 3 - Convert Aggregate Size And Addressing Consumers converted local GEP
raw-byte scalar leaf discovery to prefer structured scalar byte-offset facts
when the optional backend structured layout table is available.

Changed files:

- `src/backend/bir/lir_to_bir/memory/memory_helpers.hpp`
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp`
- `src/backend/bir/lir_to_bir/memory/local_gep.cpp`
- `todo.md`
- `test_after.log`

Converted consumers: `resolve_scalar_layout_facts_at_byte_offset()` now has a
structured-aware overload that uses `lookup_backend_aggregate_type_layout()`
when a structured table is supplied and keeps the original three-argument
legacy helper as a `nullptr` fallback wrapper. The recursive scalar leaf walker
uses the same lookup path for nested array elements and struct fields.

Converted call sites: `resolve_local_aggregate_raw_byte_slice_leaf()` now passes
its optional structured table into scalar byte-offset fact resolution, so the
raw-byte slice fast path used by local aggregate GEP slot and target discovery
uses structured layout when available.

Remaining fallback-only paths: legacy callers of the three-argument scalar
byte-offset helper intentionally keep `compute_aggregate_type_layout()`;
`collect_local_scalar_array_slots()` still uses the legacy local-slots helper;
`provenance.cpp` still calls the legacy scalar byte-offset helper; static
global-address helper wrappers, ABI and call lowering, globals, initializers,
local slot load/store-heavy lowering, and unrelated local slot paths remain
outside this packet.

## Suggested Next

Next coherent conversion packet: convert `collect_local_scalar_array_slots()` or
the remaining provenance scalar byte-offset query with the same
structured-when-available, legacy-wrapper-fallback shape, if the supervisor
wants to continue Step 3 before broader review.

## Watchouts

`ENABLE_C4C_BACKEND` remains off in the current `default` build tree, so the
first delegated build may report no work for backend objects. The delegated
proof includes the backend-enabled `c4c_backend` target.

Do not convert ABI, globals, initializers, local slots, or load/store-heavy
consumers unless the supervisor explicitly widens ownership.

## Proof

Delegated proof passed and wrote `test_after.log`:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|positive_split_llvm_|abi_)' && cmake --build build-backend --target c4c_backend -j) > test_after.log 2>&1`

Result: default build completed with no work, selected ctest subset passed 8/8,
and `build-backend` rebuilt the backend objects needed by `c4c_backend`
successfully.
