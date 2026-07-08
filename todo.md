Status: Active
Source Idea Path: ideas/open/599_pointer_base_plus_offset_selected_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Pointer-Arithmetic Authority

# Current Packet

## Just Finished

Completed Step 2 from `plan.md`: defined the selected pointer-arithmetic
authority contract for the representative shared store-source publication
route.

Representative route being authorized:

- Shared route:
  `PreparedStoreSourcePublicationPlan` /
  `plan_prepared_store_source_publication(...)` when the source home is
  `PreparedValueHomeKind::PointerBasePlusOffset`.
- Current target-side consumer:
  AArch64 `plan_pointer_base_plus_offset_store_local_publication(...)` /
  `lower_pointer_base_plus_offset_store_local_publication(...)`.
- Authorized use:
  accepting the computed pointer result as the source value for store-local
  publication materialization. This is not pointer-value indirect memory-use
  freshness and does not authorize broad target operand formation.

Freshness vocabulary decision:

- A distinct pointer-arithmetic vocabulary entry is required. Existing
  `CallArgumentSource`, `MoveBundleSource`, `ProducerPublicationOperand`,
  `DirectEdgePublicationSource`, `BranchStackLoadSource`, and
  `SelectCarrierAliasSource` do not own this boundary because they prove call
  argument availability, move bundle source availability, producer operand
  availability, edge move publication, branch stack load ordering, or select
  carrier aliasing. None ties a computed pointer result to its base pointer,
  byte delta, and store-source use program point.
- Step 3 should add narrow enum cases and contract matching:
  `PreparedValueFreshnessUseKind::PointerBasePlusOffsetSource`,
  `PreparedValueFreshnessSourceKind::PointerBasePlusOffset`,
  `PreparedValueFreshnessProofKind::PointerBasePlusOffsetAuthority`, and
  `PreparedValueFreshnessSourceRank::PointerBasePlusOffset`. Suggested rank
  placement is a distinct route-specific rank near other selected semantic
  source ranks; the exact numeric value only needs to be unique and stable.

Query/reference contract:

- Query `value_id` and `value_name` name the computed pointer result carried by
  the `PointerBasePlusOffset` source home.
- Query `use_kind` is `PointerBasePlusOffsetSource`.
- Query program point is the store-source publication use site: the current
  block/index and instruction index for the store-local publication route.
- The selected authority must have source kind
  `PointerBasePlusOffset`, proof kind `PointerBasePlusOffsetAuthority`, and
  rank `PointerBasePlusOffset`.
- The selected authority reference must point at the exact
  `PreparedValueHome` for the computed pointer and must carry the same
  program-point reference used by the query. The matching helper must require
  home kind `PointerBasePlusOffset`, matching result `value_id`/`value_name`,
  matching `pointer_base_value_name`, matching optional base symbol when
  present, matching `pointer_byte_delta`, and matching block/instruction
  reference.
- Base pointer freshness is represented by the selected pointer-arithmetic
  authority over the coherent source home, not by target placement. The base
  identity comes from `pointer_base_value_name`; Step 3 must not substitute a
  register/stack home for the base as freshness.

Facts insufficient by themselves:

- Home shape, byte delta, optional base symbol, stack/register placement,
  layout/range proof, signed-12-bit encodability, target offset encodability,
  target operand shape, diagnostics, prepared dumps, structural contract
  coherence, decoded-home classification, and store-source publication support
  fields are all insufficient without selected pointer-arithmetic authority.
- A `PreparedStoreSourcePublicationPlan` with
  `prepared_store_source_publication_available(plan) == true` remains support
  evidence only until the selected pointer-arithmetic freshness helper accepts
  the exact source/result/base/delta/use/program-point authority.

Expected fail-closed statuses/diagnostics:

- Missing/no-candidate freshness: `missing_pointer_arithmetic_source_freshness`
  or the existing generic `missing_source_freshness_authority` if Step 3 keeps
  the current status vocabulary narrow.
- Ambiguous candidates: `ambiguous_pointer_arithmetic_source_freshness` or
  generic `ambiguous_source_freshness_authority`.
- Stale/wrong program point: invalid freshness, reported distinctly as
  `stale_pointer_arithmetic_source_freshness` if a new status is added, or
  generic `invalid_source_freshness_authority` with the helper rejecting the
  block/instruction mismatch.
- Wrong base, wrong result, wrong delta, or wrong use: invalid freshness; the
  selected helper must reject the route before target materialization.
- Range-only, target-shape-only, and support-only routes: missing or invalid
  pointer-arithmetic freshness, never successful source acceptance.

## Suggested Next

Execute Step 3 from `plan.md`: migrate the representative
`PreparedStoreSourcePublicationPlan` / AArch64 store-local publication route to
require selected `PointerBasePlusOffsetSource` freshness before accepting a
`PointerBasePlusOffset` source home.

Step 3 should add the distinct vocabulary and helper in the shared prealloc
layer, centered on `src/backend/prealloc/value_locations.hpp`,
`src/backend/prealloc/prepared_lookups.cpp`, and
`src/backend/prealloc/publication_plans.hpp/.cpp`. The target-side AArch64
route should consume that helper result; it should not grow target-local
freshness semantics.

## Watchouts

- Keep pointer-value indirect memory-use freshness in idea 600.
- Do not treat home shape, byte delta, range/layout facts, stack/register
  placement, target offset encodability, target operand shape, diagnostics, or
  dumps as selected pointer-arithmetic authority.
- The selected route should be shared-prealloc first:
  `PreparedStoreSourcePublicationPlan` / `plan_prepared_store_source_publication(...)`.
  AArch64 memory lowering can consume that result, but target-local
  materialization must not become the semantic authority.
- If `PreparedStoreSourcePublicationPlan` lacks the block/instruction
  reference needed by the exact query, Step 3 should add the minimal reference
  fields required for this route instead of falling back to target shape.
- The selected helper should fail closed for missing, ambiguous,
  stale/wrong-program-point, wrong-base, wrong-result, wrong-delta, wrong-use,
  range-only, target-shape-only, and support-only evidence.
- Target paths explicitly out of scope for this runbook: RV64 edge publication,
  scalar emit, frame/context helpers, and object-emission diagnostics; AArch64
  generic operand resolution, call lowering, and broad memory lowering beyond
  the representative store-local consumer; x86 module lowering/rejections;
  semantic GEP target consumption; relocation/materialization semantics.
- Do not reuse branch, edge-publication, move-bundle, select-carrier, alias,
  call-argument, producer-publication, or pointer-value memory-use freshness
  vocabulary for this route.

## Proof

Contract-only/todo-only packet. No build or tests were run, and
`test_after.log` was not updated. Proof command: `git diff --check`.
