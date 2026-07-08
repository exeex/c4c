Status: Active
Source Idea Path: ideas/open/600_pointer_value_memory_use_freshness_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Pointer-Value Memory Freshness Authority

# Current Packet

## Just Finished

Completed Step 2 from `plan.md`: defined the selected pointer-value memory-use
freshness authority contract for shared `PreparedMemoryAccess` records whose
address has `PreparedAddressBaseKind::PointerValue`, initially consumed by
AArch64 prepared memory operand formation.

Authority vocabulary decision:

- Add distinct route-specific vocabulary; do not reuse call argument,
  move-bundle, producer-publication, direct-edge-publication, branch-stack,
  select-carrier, pointer-base-plus-offset, loaded-value, or store-source
  freshness.
- Required names for Step 3:
  `PreparedValueFreshnessUseKind::PointerValueMemoryUse`,
  `PreparedValueFreshnessSourceKind::PointerValueMemoryAccess`,
  `PreparedValueFreshnessProofKind::PointerValueMemoryAuthority`, and
  `PreparedValueFreshnessSourceRank::PointerValueMemory`.
- The authority value is the pointer value used as the memory address base, not
  the loaded value and not the stored source value.

Selected-authority dimensions:

- Pointer identity: match the exact prepared pointer value id/name resolved
  from `PreparedMemoryAccess::address.pointer_value_name`; missing id/name is
  no authority.
- Memory use: match the exact access mode derived from the access record
  (`result_value_name` for load, `stored_value_name` for store). A load
  authority cannot authorize a store use and a store authority cannot authorize
  a load use.
- Program point: match the exact `function_name`, `block_label`, and
  `inst_index` of the `PreparedMemoryAccess`; stale or wrong-block/wrong-index
  evidence must not select.
- Offset/range coordinate: the access byte offset, size, alignment,
  `can_use_base_plus_offset`, requested range, and range verdict are required
  support dimensions for the candidate, but they are not the selected
  freshness value by themselves.
- Provenance/layout support: provenance base identity, object extent,
  layout authority, address space, and volatility must match the access being
  authorized. They support the memory-use authority and remain insufficient
  alone.
- Target shape: target offset encodability, target memory operand kind,
  register/storage placement, and final target operand formation are rejected
  as authority; targets may only consume the shared selected result.

Facts insufficient by themselves:

- `PreparedAddressBaseKind::PointerValue`, pointer value name/id, object
  extent, range proof, layout authority, local layout, target offset
  encodability, target memory operand shape, diagnostics, prepared dumps, edge
  publication source-memory facts, store-source destination facts, and
  `prepared_pointer_value_memory_has_proven_authority(...)`.

Fail-closed contract:

- Missing/no-candidate authority: query status `NoCandidate` and a
  route-specific missing pointer-value memory freshness diagnostic/status.
- Ambiguous authority: query status `AmbiguousCandidate` and an ambiguous
  pointer-value memory freshness diagnostic/status.
- Stale/wrong program point: no selected authority when function, block label,
  or instruction index differs.
- Wrong pointer: no selected authority when pointer value id/name differs from
  the access base pointer.
- Wrong load/store use: no selected authority when the candidate mode differs
  from the access mode.
- Wrong vocabulary, missing reference, or wrong source/proof/rank:
  `InvalidCandidate`.
- Range-only, layout-only, target-shape-only, and support-only routes:
  `NoCandidate`; these facts can explain address legality but cannot authorize
  pointer freshness.

## Suggested Next

Execute Step 3 from `plan.md`: add the distinct pointer-value memory-use
freshness vocabulary and a narrow shared helper, likely
`prepared_pointer_value_memory_freshness_available(...)`, adjacent to
`PreparedMemoryAccess`/prepared lookup support. Publish/query the selected
authority from existing prepared access facts, then have the representative
AArch64 prepared memory operand route consume that shared selected result.

## Watchouts

- Step 3 may need to add a minimal `PreparedMemoryAccess` reference or
  route-specific freshness wrapper because `PreparedValueFreshnessAuthority`
  currently has value/use/program-point fields but no explicit memory-access
  mode, offset/range, or provenance coordinate.
- Keep loaded-value freshness, store-source freshness, pointer arithmetic,
  semantic GEP target consumption, global symbol memory freshness, and broad
  RV64/AArch64/x86 target migration out of this route.
- Do not make AArch64 target operand shape the semantic owner. The target route
  should reject or decline when the shared selected pointer-value memory-use
  authority is absent.

## Proof

Contract-only/todo-only packet. Build/tests were not run and `test_after.log`
was not updated. Proof command: `git diff --check` passed.
