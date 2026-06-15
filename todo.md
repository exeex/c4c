Status: Active
Source Idea Path: ideas/open/279_phase_f5_x86_memory_accesses_public_field_parity_gate.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect x86 memory_accesses Consumers

# Current Packet

## Just Finished

Completed Step 1 inspection for the x86 `memory_accesses` public-field parity
gate. Examined `src/backend/mir/x86/prepared/dispatch.cpp`,
`src/backend/mir/x86/module/module.cpp`, `src/backend/mir/x86/x86.hpp`,
`src/backend/prealloc/addressing.hpp`, and the adjacent x86 fixture tests
`backend_x86_prepared_decoded_home_storage_test.cpp` and
`backend_x86_handoff_boundary_joined_branch_test.cpp`.

No real x86 public `PreparedFunctionLookups::memory_accesses` consumer boundary
was found. The real x86 prepared-module source-memory surface is the
`LoadLocal` Route 3 / Route 5 agreement path in `module.cpp`: it consumes
`ConsumedPlans::shared_function_lookups()` for `edge_publications`, compares
Route 3 memory records against `PreparedEdgePublication::source_memory_access`,
and renders/fails closed through `PreparedAddressingFunction` /
`find_prepared_memory_access(addressing, ...)`. The x86 prepared dispatch helper
also consumes `lookups->edge_publications` only. Direct search and AST-backed
caller/callee checks found no x86 backend flow reading
`lookups->memory_accesses`.

## Suggested Next

Execute Step 2 as a bounded non-applicability packet: record that no x86
public-field proof row is currently supportable because the absent boundary is
an x86 backend consumer of `PreparedFunctionLookups::memory_accesses`. The
smallest implementation prerequisite for a future parity proof would be to add
or expose a real x86 consumer boundary that reads the public memory-access
lookup, then prove one row through that path.

## Watchouts

Do not claim x86 parity from the existing x86 Route 3 / Route 5 source-memory
agreement tests: they are real backend consumers, but they consume
`edge_publications`, `PreparedEdgePublication::source_memory_access`, and
`PreparedAddressingFunction`, not the public `memory_accesses` lookup field.
Do not use helper-only lookup tests, prepared-printer output, wrappers,
copied-publication paths, or renamed helpers as substitutes for the missing
boundary.

## Proof

Inspection-only packet. No build or tests were run, and `test_before.log` /
`test_after.log` were not touched. Used `rg`, targeted file reads, and
`c4c-clang-tool-ccdb` caller/callee checks on the x86 module.
