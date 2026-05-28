Status: Active
Source Idea Path: ideas/open/61_aarch64_shared_same_block_materialization_authority.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Same-Block Discovery

# Current Packet

## Just Finished

Step 1 - Inventory Same-Block Discovery completed without implementation edits.

Inventory:

- `dispatch_lookup.cpp`:
  - `find_same_block_scalar_producer` is local semantic discovery. It scans
    `context.bir_block->insts` backward before `before_instruction_index`,
    accepts only named `bir::BinaryInst` results, and filters opcodes through
    `is_scalar_call_argument_producer_opcode` (`Add`, `Sub`, `And`, `Or`,
    `Xor`, `Mul`, `SDiv`, `SRem`). Shared owner candidate:
    `prepare::find_prepared_same_block_scalar_producer` backed by
    `PreparedEdgePublicationSourceProducerLookups`.
  - `has_same_block_load_local_producer` is local semantic discovery. It scans
    same-block `bir::LoadLocalInst` results by value spelling. Shared owner
    candidate: `prepare::find_prepared_same_block_load_local_source_producer`
    when memory-access facts are needed, or the same scalar producer query when
    only the source-producer kind/index is needed.
  - `make_named_prepared_result_register` and
    `emitted_scalar_value_available` are target/register availability lookups,
    not same-block producer discovery.

- `dispatch_value_materialization.cpp`:
  - `prepared_same_block_scalar_producer` is already a local wrapper over
    shared prepared authority. It resolves the prepared value name, uses
    `context.function.prepared_lookups->edge_publication_source_producers`
    when present, otherwise builds the lookup table, and calls
    `prepare::find_prepared_same_block_scalar_producer`.
  - `value_power_of_two_shift` still uses raw shared MIR query
    `mir::evaluate_same_block_integer_constant(context.bir_block, value)`.
    That query scans same-block binary producers by BIR value spelling. Shared
    owner candidate exists internally as
    `evaluate_prepared_same_block_integer_constant`, but no public prepared
    query is currently exposed for AArch64 value materialization.
  - `value_publication_may_write_scratch_register` uses the prepared scalar
    producer wrapper to classify `Cast`, `Binary`, `LoadGlobal`, `LoadLocal`,
    and `SelectMaterialization` source identity, but it also consults raw
    `mir::evaluate_same_block_integer_constant`; the latter is a missing
    prepared integer-constant fact gap.
  - `emit_value_publication_to_register` is mixed authority plus target
    emission. Target-only responsibilities include register view/name
    selection, scratch selection, instruction spelling, stack/global/local
    load emission, recursive sequencing, and reload policy. Source identity is
    currently prepared for scalar producers, same-block global-load access, and
    narrow-store recovery, but integer constants still use raw MIR discovery.
  - Integer constants: immediate values are target emission only; named
    constants use raw `mir::evaluate_same_block_integer_constant` and need a
    public prepared integer-constant query before the fallback can be removed.
  - ALU: `Add`, `Sub`, `And`, `Or`, `Xor`, and `Mul` emission relies on the
    prepared scalar producer for the result identity, then recursively emits
    operands. Power-of-two `Mul` optimization depends on the missing prepared
    integer-constant fact.
  - Comparison: comparison-result publication uses prepared scalar producer
    identity for the result, but integer compare operands can recurse through
    the same mixed path; prepared comparison helpers already exist in
    `prepare::find_prepared_fused_compare_operand_producer` and
    `prepare::find_prepared_materialized_condition_producer`.
  - FP comparison publication keeps target FP scratch/register emission local
    and delegates operand materialization to `emit_fp_value_to_register`; nearby
    FP materialization already has prepared scalar/global-load owner candidates
    through `prepare::find_prepared_same_block_scalar_producer` and
    `prepare::find_prepared_same_block_global_load_access`.
  - Load-local publication uses prepared scalar producer identity for the load,
    prepared memory access checks, prepared value homes, va-list/global-symbol
    special cases, narrow-store recovery, frame-slot offset, and pointer-load
    emission. The same-block load-local source/stored-value owner candidate is
    `prepare::find_prepared_same_block_load_local_stored_value_source`.
  - Negative cases already fail closed by returning `false`/`std::nullopt` for
    missing prepared name, block/control-flow context, producer mismatch,
    invalid index bounds, missing value home, missing memory access, bad
    register view/name, unknown producer kind, recursion depth limits, and
    unsupported producer opcode.

- `dispatch_producers.*`:
  - `prepared_source_producer_for_value` and
    `prepared_same_block_select_producer` are prepared-owner wrappers over
    `PreparedEdgePublicationSourceProducerLookups`; they validate block label,
    instruction index, pointer identity, result name, and type.
  - `find_same_block_select_producer` still falls back to raw
    `mir::find_same_block_select_producer` when the prepared wrapper has no
    fact. This fallback is local same-block semantic discovery and should be
    removed or made fail-closed once prepared facts are authoritative.
  - `find_prepared_same_block_select_producer` is target/query wrapper only; it
    does not fall back to raw BIR scanning.
  - `prepared_select_chain_contains_direct_global_load` is prepared recursive
    query logic. `select_chain_contains_direct_global_load` falls back to raw
    `mir::select_chain_contains_dependency`; the active plan marks direct-global
    select-chain migration out of scope, so preserve this as a negative
    non-goal unless a later lifecycle step explicitly moves it.
  - `is_current_block_join_parallel_copy_source`,
    `is_current_block_join_parallel_copy_incoming_expression`, and
    `build_current_block_join_parallel_copy_cache` scan current-block BIR and
    move bundles for join-copy expression/source classification. The plan
    marks join-copy query migration out of scope; preserve existing behavior
    and do not expand it as part of same-block scalar materialization.
  - `find_same_block_result_index` and `same_block_result_depends_on_value`
    are local recursive expression-dependency discovery used by join-copy
    incoming-expression logic; out of scope for this migration unless a
    separate plan owns join-copy facts.

Shared prepared/query owner candidates:

- General same-block source identity: `PreparedEdgePublicationSourceProducer`
  and `PreparedEdgePublicationSourceProducerLookups`.
- Scalar result producer: `prepare::find_prepared_same_block_scalar_producer`.
- Load-local source/stored-value facts:
  `prepare::find_prepared_same_block_load_local_source_producer` and
  `prepare::find_prepared_same_block_load_local_stored_value_source`.
- Global-load access from same-block producer:
  `prepare::find_prepared_same_block_global_load_access`.
- Comparison source facts:
  `prepare::find_prepared_fused_compare_operand_producer` and
  `prepare::find_prepared_materialized_condition_producer`.
- Select-chain/source wrapper candidate:
  `find_prepared_same_block_select_producer` over the same prepared source
  producer lookups.

Missing shared fact gaps:

- Public prepared integer-constant evaluation for value materialization and
  power-of-two checks. The implementation exists internally in
  `prepared_lookups.cpp` as `evaluate_prepared_same_block_integer_constant`,
  but it is not declared as a reusable prepared query.
- A dispatch-level shared wrapper for same-block scalar producer lookup is
  duplicated between value materialization, FP value materialization, and other
  consumers; a shared prepared/query facade would avoid each target helper
  rebuilding or directly probing the lookup table.
- Same-block load-local availability in `dispatch_lookup.cpp` lacks a prepared
  replacement that answers the narrow "does this named value have a prior
  same-block LoadLocal producer?" question without requiring a raw scan.
- Raw select fallback in `find_same_block_select_producer` remains a semantic
  discovery path, but direct-global select-chain and join-copy migration are
  explicit non-goals for the current plan.

Negative/fail-closed cases to preserve:

- Missing `prepared`, `prepared_lookups`, `control_flow`, `control_flow_block`,
  `bir_block`, prepared value name, value home, memory access, addressing, or
  register view/name must return unavailable/false rather than scan locally.
- Prepared producer facts with mismatched block label, instruction index after
  the use, out-of-range index, mismatched pointer, mismatched result name/type,
  `Unknown` kind, or unsupported opcode must fail closed.
- Recursion limits for scalar publication, select-chain traversal, and integer
  constant folding must continue to stop recursive discovery.
- Out-of-scope raw routes for edge-copy dependency fallback, direct-global
  select-chain fallback, and join-copy expression classification should not be
  broadened under this idea.

## Suggested Next

Step 2 - Define Shared Same-Block Source Facts: expose the missing prepared
integer-constant query and a prepared same-block scalar/load-local source-fact
facade that AArch64 can consume without raw BIR scans.

## Watchouts

Do not migrate by adding a new AArch64 wrapper around
`mir::evaluate_same_block_integer_constant`,
`mir::find_same_block_select_producer`, or local `LoadLocal` scans. The next
code-changing packet should make missing prepared facts fail closed rather than
fall back to target-local semantic discovery.

## Proof

`printf 'Inventory-only Step 1; no build or test proof required because no implementation files changed.\n' > test_after.log`

No build or test subset required by the delegated packet because this was an
inventory-only slice with no implementation edits. Proof log:
`test_after.log`.
