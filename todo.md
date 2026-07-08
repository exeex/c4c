Status: Active
Source Idea Path: ideas/open/600_pointer_value_memory_use_freshness_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate One Representative Consumer

# Current Packet

## Just Finished

Implemented Step 3 from `plan.md`: added narrow pointer-value memory-use
freshness vocabulary and a shared route helper, then made the AArch64 prepared
load/store instruction-record path query that helper before resolving a
pointer-value base register.

Files changed:

- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/pointer_value_memory_freshness.hpp`
- `src/backend/mir/aarch64/codegen/instruction.hpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `todo.md`

Key semantic boundary:

- New vocabulary: `PointerValueMemoryUse`,
  `PointerValueMemoryAccess`, `PointerValueMemoryAuthority`, and
  `PointerValueMemory`.
- The selected authority value is the pointer address base id/name, not the
  loaded result and not the stored source value.
- The shared helper matches pointer id/name, load/store mode, function/block
  instruction point, byte offset, size/alignment, address space, volatility,
  provenance base facts, layout/range support, and source/proof/rank.
- The helper treats `PreparedValueId{0}` as a valid prepared value id when the
  optional pointer home is present; this keeps sret pointer-value destination
  stores under the same pointer-value memory-use contract.
- `prepared_pointer_value_memory_has_proven_authority(...)`, range/layout
  proof, local layout, target operand shape, edge source-memory facts, and
  store-source destination facts remain support only; they are not accepted by
  the helper without selected pointer-value memory freshness.
- The representative AArch64 path consumes the shared helper before
  pointer-value base register resolution; target operand shape does not own
  the semantic freshness decision.

## Suggested Next

Execute Step 4 from `plan.md`: prove the migrated route fails closed without
selected pointer-value memory-use freshness and does not accept range, layout,
target-shape, or support-only evidence as freshness authority.

## Watchouts

- `SretParameter` provenance and prepared value id `0` are included under the
  same pointer-value memory-use rule when the prepared access names the pointer
  value and the selected authority matches the exact store use.
- Do not broaden this packet into loaded-value freshness, store-source
  freshness, pointer arithmetic, semantic GEP target consumption, global symbol
  memory freshness, or broad target migration.

## Proof

`git diff --check` passed.

Required proof command was run and wrote `test_after.log`:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: build completed; CTest passed 346/346.
