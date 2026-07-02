# BIR Local-Memory Producer Boundary

Status: Step 1 evidence-only producer-boundary packet for
`ideas/open/557_bir_local_memory_semantic_producer_admission.md`.

## Boundary Decision

The concrete producer boundary for the current local-memory admission load is
the semantic LIR-to-BIR lowering dispatch in
`src/backend/bir/lir_to_bir/memory/coordinator.cpp`, with fact production in
the family-specific `BirFunctionLowerer` memory methods under
`src/backend/bir/lir_to_bir/memory/`.

The boundary is not prepared-object publication, RV64/MIR object lowering, or
runtime comparison. Current rows fail with `semantic lir_to_bir` first-topic
diagnostics before prepared object handoff, and existing BIR/RV64 tests require
prepared/RV64 consumers to reject missing or incomplete memory facts instead of
inferring them from instruction shape.

## Local-Memory Rows

Evidence source:
`docs/rv64_gcc_torture_post_contract/bir_semantic_admission_classification.md`
classifies `264` exact semantic rows as BIR local-memory producer rows:

| Family | Rows | Representative seed |
| --- | ---: | --- |
| `load local-memory semantic family` | 79 | `src/20000314-1.c` |
| `gep local-memory semantic family` | 62 | `src/20000717-4.c` |
| `store local-memory semantic family` | 58 | `src/20001026-1.c` |
| `scalar/local-memory semantic family` | 49 | `src/20000519-1.c` |
| `alloca local-memory semantic family` | 16 | `src/20050604-1.c` |

The inspected current row table also records these representative rows as
exact semantic rows with visible first-topic evidence:

| Seed | Function | First visible topic | Case log |
| --- | --- | --- | --- |
| `src/20000314-1.c` | `main` | `load local-memory semantic family` | `build/rv64_gcc_c_torture_backend/src_20000314-1.c/case.log` |
| `src/20000717-4.c` | `x` | `gep local-memory semantic family` | `build/rv64_gcc_c_torture_backend/src_20000717-4.c/case.log` |
| `src/20001026-1.c` | `build_real_from_int_cst_1` | `store local-memory semantic family` | `build/rv64_gcc_c_torture_backend/src_20001026-1.c/case.log` |
| `src/20000519-1.c` | `foo` | `scalar/local-memory semantic family` | `build/rv64_gcc_c_torture_backend/src_20000519-1.c/case.log` |
| `src/20050604-1.c` | `foo` | `alloca local-memory semantic family` | `build/rv64_gcc_c_torture_backend/src_20050604-1.c/case.log` |

## Producer Symbols

Top-level pipeline:

- `src/backend/bir/lir_to_bir.cpp:594`:
  `try_lower_to_bir_with_options(...)` builds lowering context, runs
  `analyze_module(...)`, runs `lower_module(...)`, and returns the lowering
  notes that carry semantic admission diagnostics.
- `src/backend/bir/lir_to_bir.cpp:619`:
  `lower_to_bir(...)` throws when the semantic module is absent; this is an
  outer API boundary, not the local-memory producer.

Dispatch and failure-family boundary:

- `src/backend/bir/lir_to_bir/memory/coordinator.cpp:25`:
  `BirFunctionLowerer::lower_scalar_or_local_memory_inst(...)`.
- `src/backend/bir/lir_to_bir/memory/coordinator.cpp:73`:
  alloca failure note is `alloca local-memory semantic family`.
- `src/backend/bir/lir_to_bir/memory/coordinator.cpp:77`:
  GEP failure note is `gep local-memory semantic family`.
- `src/backend/bir/lir_to_bir/memory/coordinator.cpp:81`:
  store failure note is `store local-memory semantic family`.
- `src/backend/bir/lir_to_bir/memory/coordinator.cpp:85`:
  load failure note is `load local-memory semantic family`.
- `src/backend/bir/lir_to_bir/memory/coordinator.cpp:292`:
  `LirAllocaOp` dispatches to `lower_local_memory_alloca_inst(...)`.
- `src/backend/bir/lir_to_bir/memory/coordinator.cpp:296`:
  `LirGepOp` dispatches to `lower_memory_gep_inst(...)`.
- `src/backend/bir/lir_to_bir/memory/coordinator.cpp:305`:
  `LirStoreOp` dispatches to `lower_memory_store_inst(...)`.
- `src/backend/bir/lir_to_bir/memory/coordinator.cpp:309`:
  `LirLoadOp` dispatches to `lower_memory_load_inst(...)`.

Concrete local-memory producers:

| Topic | Producer boundary | Main fact shape owned here |
| --- | --- | --- |
| Alloca | `BirFunctionLowerer::lower_local_memory_alloca_inst(...)` in `memory/local_slots.cpp:606` | Creates BIR local slots, local array source object records, dynamic alloca pointer-address facts, and aggregate local slots. |
| GEP | `BirFunctionLowerer::lower_memory_gep_inst(...)` in `memory/addressing.cpp:1085` | Resolves local/global address derivations, local array path/source-object records, pointer aliases, dynamic local/global array accesses, and producer-coordinate metadata. |
| Store | `BirFunctionLowerer::lower_memory_store_inst(...)` in `memory/local_slots.cpp:699` | Emits `StoreLocalInst`/`StoreGlobalInst`, local aggregate copies, dynamic array stores, pointer-provenance stores, local scalar slot cache updates, and address/provenance side tables. |
| Load | `BirFunctionLowerer::lower_memory_load_inst(...)` in `memory/local_slots.cpp:1001` | Emits `LoadLocalInst`/`LoadGlobalInst`, aggregate loads, dynamic array loads, pointer-provenance loads, loaded scalar immediates, pointer aliases, and memory address/provenance facts. |
| Scalar/local-memory | Same dispatcher plus memory-path scalar cases in `lower_scalar_or_local_memory_inst(...)`, `lower_memory_load_inst(...)`, `lower_memory_store_inst(...)`, `lower_memory_gep_inst(...)`, and intrinsic local memory helpers | Covers scalar values whose admission depends on local-memory facts, including byte-slice, partial memcpy, pointer cast/address-int, and scalar slot state. |

Supporting producer helpers:

- `src/backend/bir/lir_to_bir/memory/local_slots.cpp:1597`:
  `try_lower_local_slot_store(...)`.
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp:1913`:
  `try_lower_nonpointer_local_slot_load(...)`.
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp:1937`:
  `try_lower_local_slot_load(...)`.
- `src/backend/bir/lir_to_bir/memory/provenance.cpp:833`:
  `try_lower_pointer_provenance_store(...)`.
- `src/backend/bir/lir_to_bir/memory/provenance.cpp:892`:
  `try_lower_addressed_pointer_store(...)`.
- `src/backend/bir/lir_to_bir/memory/provenance.cpp:951`:
  `try_lower_pointer_provenance_load(...)`.
- `src/backend/bir/lir_to_bir/memory/provenance.cpp:1034`:
  `try_lower_addressed_pointer_load(...)`.
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp:2277` and nearby overloads:
  `load_dynamic_local_aggregate_array_value(...)`.
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp:2354` and nearby overloads:
  `append_dynamic_local_aggregate_store(...)`.
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp:2458`:
  `try_lower_dynamic_local_aggregate_store(...)`.
- `src/backend/bir/lir_to_bir/memory/local_slots.cpp:2495`:
  `try_lower_dynamic_local_aggregate_load(...)`.

## Existing Contract Anchors

`tests/backend/bir/backend_lir_to_bir_notes_test.cpp` already checks that
semantic LIR-to-BIR notes classify these families before handoff:

- `bad_alloca`: `alloca local-memory semantic family`.
- `bad_gep`: `gep local-memory semantic family`.
- `bad_store`: `store local-memory semantic family`.
- `bad_load`: `load local-memory semantic family`.
- Admitted local-memory lanes such as
  `local_aggregate_raw_i8_gep_byte_slice`,
  `local_aggregate_raw_float_leaf_byte_slice`,
  `local_scalar_double_decimal_zero_store`,
  `local_scalar_double_partial_float_memcpy`,
  `local_scalar_i64_partial_i8_memcpy`, and
  `dynamic_indexed_gep_local_member_array` assert that successful producer
  work should not continue to report the old local-memory family note.

`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp` anchors
the consumer-side guardrail:

- Available shared memory facts let RV64 consume a dynamic stack-source load.
- Missing or incomplete prepared memory access rows are rejected.
- The RV64 helper explicitly must not infer a load from `LoadLocal` shape or
  pointer decorations.

That test shape confirms the next repair should publish the missing BIR facts
at the producer methods above, not infer them downstream.

## Commands Inspected

```sh
git status --short
sed -n '1,220p' todo.md
sed -n '1,220p' docs/rv64_gcc_torture_post_contract/bir_semantic_admission_classification.md
sed -n '1,220p' docs/rv64_gcc_torture_post_contract/bir_semantic_admission_rows.md
sed -n '1,220p' docs/rv64_gcc_torture_post_contract/bir_semantic_admission_followups.md
sed -n '1,220p' docs/rv64_gcc_torture_post_contract/bir_semantic_admission_outcome.md
command -v c4c-clang-tool && command -v c4c-clang-tool-ccdb
c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/bir/lir_to_bir.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/bir/lir_to_bir.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/bir/lir_to_bir/memory/coordinator.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/bir/lir_to_bir/memory/local_slots.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/bir/lir_to_bir/memory/local_gep.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/bir/lir_to_bir/memory/value_materialization.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/bir/lir_to_bir/memory/addressing.cpp build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/bir/lir_to_bir/memory/coordinator.cpp lower_scalar_or_local_memory_inst build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/bir/lir_to_bir/memory/local_slots.cpp lower_memory_load_inst build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/bir/lir_to_bir/memory/local_slots.cpp lower_memory_store_inst build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/bir/lir_to_bir/memory/addressing.cpp lower_memory_gep_inst build/compile_commands.json
c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/bir/lir_to_bir/memory/local_slots.cpp lower_local_memory_alloca_inst build/compile_commands.json
rg -n 'lower_scalar_or_local_memory_inst|lower_local_memory_alloca_inst|lower_memory_gep_inst|lower_memory_store_inst|lower_memory_load_inst|note_function_lowering_family_failure|failed in .*local-memory|source_memory_access_status|should not infer' src/backend/bir/lir_to_bir/memory src/backend/bir/lir_to_bir.cpp tests/backend/bir/backend_lir_to_bir_notes_test.cpp tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp
```

The `function-callees` commands for `BirFunctionLowerer` member methods did
not resolve the queried unqualified names in those translation units. The
evidence above therefore uses AST-backed function signature and symbol
inventory where available, then targeted source ranges for the member-method
dispatch and fact-producing branches.

## Next Focused Packet

Use focused BIR tests before any implementation repair. The next packet should
build or extend `tests/backend/bir/backend_lir_to_bir_notes_test.cpp` style
fixtures that assert producer-side fact publication for:

1. `LoadLocalInst` with complete `MemoryAddress` and provenance facts for a
   representative pointer-value local load.
2. `StoreLocalInst` with matching address/provenance facts for pointer-value
   local store.
3. Local GEP derivation records for a structured local array/member path,
   including producer-coordinate metadata.
4. Alloca source-object/local-slot records for scalar, fixed local array, and
   aggregate alloca lanes.
5. Scalar/local-memory bridges for partial byte/float memory lanes and pointer
   cast/address-int state.

After focused BIR coverage exists, run a narrow RV64 gcc_torture subset using
the five representative seeds above. Do not weaken admission notes,
unsupported markers, expectations, allowlists, runtime comparison behavior, or
downstream prepared/RV64 consumer checks.
