Status: Active
Source Idea Path: ideas/open/160_bir_select_chain_global_dependency_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Select-Chain Surfaces

# Current Packet

## Just Finished

Completed Step 1 inventory for
`ideas/open/160_bir_select_chain_global_dependency_annotation_schema.md`.
The current Route 2 prepared oracle surfaces are:

- `prepare::find_prepared_select_chain_source_producer(...)` in
  `src/backend/prealloc/select_chain_lookups.*`: root producer authority for
  named BIR values, keyed by prepared value name, block label, and
  `before_instruction_index`.
- `prepare::find_prepared_direct_global_select_chain_dependency(...)`: direct
  global-load dependency oracle with
  `contains_direct_global_load`, `root_is_select`, and
  `root_instruction_index`.
- `prepare::find_prepared_scalar_select_chain_materialization(...)`: scalar
  materialization oracle with `available`, `root_value_name`,
  `root_is_select`, `root_instruction_index`, and embedded direct-global
  dependency.
- `prepare::find_prepared_call_argument_direct_global_select_chain_dependency(...)`
  plus the BIR-side
  `bir::CallArgumentDirectGlobalSelectChainDependency`: existing call-argument
  routing quarantine for direct-global select-chain facts.

The current BIR/MIR query surfaces are:

- `mir::BirSelectChainIdentityRequest`: `block`, `block_label`,
  `root_value` or `root_value_name`, `root_value_type`, and
  `before_instruction_index`.
- `mir::find_bir_select_chain_source_producer(...)`: currently backed by the
  Route 1 same-block producer index.
- `mir::find_bir_select_chain_direct_global_dependency(...)`: currently
  recursively walks `LoadGlobalInst`, `LoadLocalInst`, `SelectInst`,
  `CastInst`, and `BinaryInst`, with depth capped and missing/unsupported
  paths failing closed.
- `mir::find_bir_select_chain_scalar_materialization_eligibility(...)`: uses
  Route 1 scalar producer availability.
- `mir::find_bir_select_chain_identity(...)`: combines root producer identity,
  root value identity, `root_is_select`, `root_instruction_index`, direct
  global dependency, and scalar materialization availability.

Concrete Step 2 schema targets:

- Value annotation record, proposed name `Route2SelectChainValueRecord`, owned
  by BIR and keyed by the selected/root `bir::Value` identity. Fields should
  include `available`, `source_value` or source value identity, `root_value`,
  `root_value_name`, `root_is_select`, `root_instruction_index`,
  `scalar_materialization_available`, and explicit direct-global summary
  availability.
- Direct-global value summary, either embedded in the value record or a nested
  record, with `contains_direct_global_load`,
  `direct_load_instruction_index`, and enough load-global identity to check the
  `LoadGlobalInst` dependency without preserving prepared records.
- Instruction annotation record, proposed name
  `Route2SelectChainProducerRecord`, for root producer instructions and direct
  `LoadGlobalInst` dependency instructions. Fields should include producer
  kind for the Route 2 root surface, produced value identity,
  `instruction_index`, and for `LoadGlobalInst` dependency records
  `global_name`/`global_name_id`.
- Optional function-local index, proposed name `Route2SelectChainIndex`,
  rebuildable from the annotation records and keyed by root/source value name
  plus instruction index. It must be an acceleration structure only, not the
  semantic source of truth.

Positive oracle cases to preserve in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`:

- Select root `%query.selected` depending on `%query.loaded` via
  `SelectInst`, with `root_is_select=true`, root index `1`, direct load index
  `0`, and scalar materialization available.
- Direct `LoadGlobalInst` root `%query.direct`, with
  `root_is_select=false`, root/direct load index `2`, and scalar
  materialization available.
- Nested non-select root `%query.binary` through `BinaryInst`/`CastInst` back
  to `%query.loaded`, with root index `5`, direct load index `0`, and scalar
  materialization available.
- Local no-dependency root `%query.local`, with root index `3`, scalar
  materialization available, and explicit no direct-global dependency.

Negative/fail-closed cases to preserve:

- Query before producer: `%query.selected` before index `1` and
  `%query.direct` before index `2` must fail closed.
- Mismatched type for an existing value name must fail closed.
- Missing root value `%query.missing` must have no root producer, no direct
  dependency, no root instruction index, and no scalar materialization.
- Non-select/direct no-dependency behavior must remain explicit for
  `LoadLocalInst` roots; no direct-global dependency should be inferred from
  local roots.
- Unsupported or ambiguous recursive paths must remain absent, not partially
  available annotations.

## Suggested Next

Execute Step 2 by adding BIR-owned Route 2 annotation records and construction
helpers in `src/backend/bir/bir.hpp` and `src/backend/bir/bir.cpp`, then extend
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp` so the new BIR
records are compared against the prepared select-chain oracles without
switching production consumers.

Proposed first code-changing proof command:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$' | tee test_after.log
```

## Watchouts

- Keep Route 2 annotations target-neutral.
- Do not import AArch64 move/branch choices, materialization cost, hazards,
  register availability, publication routing, or call ABI behavior.
- Do not claim progress through prepared-helper reshuffling unless BIR owns a
  typed annotation payload.
- Preserve explicit negative cases for no direct-global dependency and
  non-select-root behavior.
- Do not encode AArch64 select-materialization instruction spelling, label
  generation, scratch register selection, materialization cost, hazard policy,
  or call ABI placement in the Route 2 BIR schema.
- The existing `CallArgumentDirectGlobalSelectChainDependency` is a useful
  migration quarantine, but Step 2 should not broaden into call consumer
  switching.
- Route 1 producer identity may be reused as an oracle or helper, but Route 2
  must introduce typed select-chain/direct-global annotation payloads rather
  than only wrapping Route 1 lookups.

## Proof

Inspection-only packet. No build/test proof was required and no
`test_after.log` was generated.

Local validation: `git diff --check`.
