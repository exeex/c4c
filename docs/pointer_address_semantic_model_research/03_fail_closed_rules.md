# 03. Fail-Closed Rules

Status: Step 4 complete

## Purpose

This file defines how pointer/address evidence must fail closed when the
semantic authority required by
`02_semantic_authority_and_fact_classes.md` is missing, ambiguous, stale, or
only represented by support, target, or diagnostic facts.

The rules are intentionally use-specific. A later implementation idea should
be able to test them by changing value identity, use kind, program point,
derivation coordinate, range authority, or fact class, not by matching one
named testcase or one target operand shape.

## Global Fail-Closed Rule

A pointer/address use is accepted only when the owning semantic authority
names the same value, the same use kind, the same program point or derivation
coordinate, and the proof required for that family. If any required dimension
is absent, conflicting, stale, or supplied only by support/target/diagnostic
facts, the consumer must reject the semantic use and either return no lowering,
keep an unavailable status, or emit an unsupported diagnostic.

Support facts may still be useful after authority exists. They must not
upgrade themselves into authority.

## Failure Mode Definitions

| Failure mode | Definition | Required fail-closed behavior |
| --- | --- | --- |
| Missing evidence | The required authority record, value identity, source object, symbol identity, derivation coordinate, range proof, freshness fact, or lookup record is absent. | Reject the use. Preserve a missing/unavailable status rather than inferring from nearby homes, layout, relocation, target operands, or diagnostics. |
| Ambiguous evidence | More than one candidate could authorize the use, or candidates disagree about value, use kind, coordinate, source, range, rank, policy, or proof. | Reject until a selected authority is unique. Do not choose by target encodability, first candidate, dump order, or shortest lowering path. |
| Stale evidence | The fact names an earlier home, block, instruction index, derivation coordinate, base pointer, or range that is not the consuming use. | Reject the use. A stale fact may be route history or diagnostics, but it cannot authorize current pointer/address semantics. |
| Wrong value | The authority names a different `ValueNameId`, derived pointer name, symbol, frame slot, source object, or branch operand than the consumer is using. | Reject even when the byte offset, range, type, or target operand shape is otherwise encodable. |
| Wrong use | The fact is valid for a different use kind, such as branch stack-load source freshness reused for a move, call, publication, load/store, GEP, or target materialization. | Reject. Semantic authority is not transferable across use kinds without a new selected authority. |
| Stack-home-only | A value has a complete stack or frame-slot home, but no use-specific freshness authority. | Reject any freshness claim. Stack homes can support memory operands only after the consuming route has its required authority. |
| Local-layout-only | A prepared local address, frame slot, layout authority, extent, offset, size, or alignment is available, but no semantic freshness or derivation authority exists. | Accept only the local memory/address route that the layout fact actually proves. Reject pointer freshness, loaded-value freshness, or unrelated address derivation. |
| Relocation-less | A global address use lacks the structured symbol/materialization record needed by the target. | Reject target materialization or lowering even if source text or diagnostics mention a symbol. |
| Relocation-only | A relocation/materialization record exists, but there is no semantic freshness, memory-access, or GEP authority for the requested pointer/address use. | Accept only target address materialization. Reject semantic pointer freshness or address-derivation claims. |
| Range-only | A range, extent, or in-bounds verdict exists without the matching value/source identity, derivation coordinate, layout authority, and use. | Reject semantic authority. Range is necessary support for address uses, not standalone pointer/address authority. |
| Target-shape-only | A backend can form a register, frame-slot, immediate, memory operand, fixup, or printed operand, but the prepared semantic authority is absent. | Reject the semantic use before emission. Do not let encodable target shape prove pointer/address validity. |
| Diagnostic-only | Dumps, unsupported messages, candidate counts, status strings, or final assembly text mention a route or value. | Treat as observation only. Diagnostics may explain rejection but cannot authorize support. |

## Family Rules

| Family | Authority required for acceptance | Failure modes to reject | Required fail-closed behavior | Later proof surface |
| --- | --- | --- | --- | --- |
| Pointer base plus offset value homes | Deferred selected pointer-arithmetic authority naming base freshness, result pointer identity, byte delta, use, and program point. | Missing evidence, ambiguous evidence, stale evidence, wrong value, wrong use, range-only, target-shape-only, diagnostic-only. | A `PreparedValueHomeKind::PointerBasePlusOffset` or normalized pointer-base-plus-offset fact remains support/target evidence only. Consumers must reject freshness or semantic use until a selected authority exists; target offset encodability cannot substitute for base freshness. | Vary base value, result value, delta, use kind, and program point; verify support facts alone do not authorize loads, branches, moves, or publication. |
| BIR pointer arithmetic materialization for frame addresses | None for freshness; materialization authority is only for the frame-address route. | Missing evidence, stale evidence, wrong value, wrong use, stack-home-only, local-layout-only, target-shape-only. | A frame-slot materialization may feed target address materialization for the exact frame address shape, but it must reject any claim that the pointed-to value or result pointer is fresh for a later use. | Vary frame slot, BIR result value, offset, and consumer use; prove materialization records do not authorize branch/call/move/load freshness. |
| Semantic relocation and global address materialization | Structured materialization record for target address formation only; separate semantic authority for any freshness or address-access use. | Missing evidence, relocation-less, relocation-only, wrong value, wrong use, target-shape-only, diagnostic-only. | Missing relocation rejects target materialization. Existing relocation accepts only materialization/fixup selection and must not authorize pointer freshness, memory-access freshness, or semantic GEP derivation. | Vary symbol identity, materialization policy, TLS/direct/GOT route, target relocation shape, and consuming semantic use. |
| Global symbol memory accesses | `PreparedMemoryAccess` whose `PreparedAddress` passes global-symbol publication authority for the exact symbol-backed memory range. | Missing evidence, ambiguous evidence, wrong value, wrong use, local-layout-only, range-only, relocation-only, target-shape-only. | Accept the exact symbol-backed memory access/range only. Reject pointer-value freshness, loaded-value freshness, store-source freshness, or unrelated GEP validity when only global access/range/relocation facts exist. | Vary symbol, offset, size, alignment, layout authority, extent, range verdict, and load/store use; prove an in-bounds range for one symbol/use does not authorize another. |
| Local stack or frame-slot addressing | Prepared local access for exact load/store address route, plus separate use-specific freshness authority for any value freshness claim. | Missing evidence, stale evidence, wrong value, wrong use, stack-home-only, local-layout-only, range-only, target-shape-only. | Frame-slot access facts may support target local memory operands, but branch/call/publication/move freshness must reject unless the selected freshness authority for that use exists. | Vary frame-slot id, stack object, offset, size, alignment, instruction use, and consuming freshness use. |
| Pointer-value indirect memory accesses | Deferred selected pointer-value memory-use freshness authority for the named pointer value at the exact load/store use, plus address legality support. | Missing evidence, ambiguous evidence, stale evidence, wrong value, wrong use, local-layout-only, range-only, target-shape-only, diagnostic-only. | `prepared_pointer_value_memory_has_proven_authority(...)`-style range/layout proof can support address legality, but it must not prove the named pointer value is fresh. Reject semantic freshness if only pointer-value base, offset, range, and encodable target memory form are present. | Vary pointer value name, load/store instruction, offset/range, provenance base, layout authority, and target offset shape. |
| Local-array source object and address derivation | `LocalArraySemanticGepRecord` with status `Available`; raw source, derivation, element path, checker input, and range proof are support. | Missing evidence, ambiguous evidence, stale evidence, wrong value, wrong use, range-only, diagnostic-only. | Raw local-array records must remain route proof. Reject semantic address derivation unless the semantic GEP evaluator reaches `Available` for the same source object, derivation, element path, dynamic index, range proof, and LIR coordinate. | Vary source object, derivation, element path, dynamic index, checker status, coordinate, element size, and bounds. |
| Local-array semantic GEP availability | `LocalArraySemanticGepRecord` status `Available` for the selected local-array address derivation. | Missing evidence, ambiguous evidence, stale evidence, wrong value, wrong use, range-only, target-shape-only. | Any non-`Available` status must remain a rejection, including missing source object, missing derivation, missing element path, coordinate confusion, missing index identity/range proof, unsupported element boundary, or out-of-bounds range. Target consumers still need their own memory/materialization facts. | Vary each evaluator precondition and verify unavailable statuses cannot be bypassed by memory layout or target operands. |
| Global static semantic GEP | `GlobalStaticSemanticGepRecord` status `Available` for the selected global static derivation. | Missing evidence, ambiguous evidence, stale evidence, wrong value, wrong use, relocation-only, range-only, target-shape-only, diagnostic-only. | Reject unless the authority has global source identity, link-name identity, source layout, derived pointer identity, layout path, element byte range, constant/dynamic range authority, proven in-bounds verdict, non-pointer element boundary, and available LIR coordinate. Relocation or final home shape is not enough. | Vary global identity, result pointer, layout path, element range, dynamic index/range authority, range verdict, element type, coordinate, and relocation route. |
| Branch pointer stack-source operands | Selected branch stack-load source freshness for the exact operand role, branch block, terminator instruction point, value/home, use kind, source kind, proof kind, and rank. | Missing evidence, ambiguous evidence, stale evidence, wrong value, wrong use, stack-home-only, target-shape-only, diagnostic-only. | RV64-style fused pointer branch emission must return no fragment or unsupported freshness diagnostics unless selected `BranchStackLoadSource` freshness is available for both required stack operands. A stack slot, frame slot, clobber-safe object, or encodable scratch-register move is insufficient alone. | Vary operand role, value/home, branch block, terminator index, freshness use/source/proof/rank, candidate count, and target operand shape. |
| Target-local operand shape | None. Target operands consume authority; they do not provide it. | Missing evidence, wrong value, wrong use, relocation-only, target-shape-only, diagnostic-only. | Printed operands, `ResolvedOperand`, target memory operands, registers, immediates, fixups, and assembly fragments must be rejected as semantic authority. They may be emitted only after the required prepared authority for the route is present. | Vary final operand form while holding semantic authority absent; verify no target shape alone unlocks pointer/address support. |

## Current Code Authority Risks

### Proven Fail-Closed Behavior

- Branch pointer stack-source RV64 emission appears to fail closed for the
  surveyed path. `selected_branch_stack_load_source_freshness_status(...)`
  requires the same role, block label, value id/name, block index,
  terminator instruction index, `BranchStackLoadSource` use kind,
  `BranchStackSlot` source/rank, `BranchTerminatorOrdering` proof, and exact
  home reference before setting `available = true`
  (`src/backend/mir/riscv/codegen/object_emission.cpp:9435`). The fused branch
  path returns no fragment when either selected operand freshness is absent
  (`src/backend/mir/riscv/codegen/object_emission.cpp:9737`).
- Local-array semantic GEP evaluation appears to fail closed for the surveyed
  semantic derivation. It returns unavailable statuses for missing source
  object, derivation, element path, LIR coordinate, dynamic index, checker
  input, range proof, scalar element shape, and bounds before setting
  `Available` (`src/backend/bir/bir_local_array_semantic_gep.hpp:2453`,
  `src/backend/bir/bir_local_array_semantic_gep.hpp:2575`).
- Global static semantic GEP evaluation appears to fail closed for the
  surveyed semantic derivation. It rejects missing global identity, layout,
  derived pointer identity, layout path, element byte range, range authority,
  in-bounds proof, pointer-element boundary, and missing LIR coordinate before
  setting `Available`
  (`src/backend/bir/bir_local_array_semantic_gep.hpp:2863`).

### No Proven Diagnostic-As-Authority Acceptance

No surveyed code path proves that diagnostic-only facts, dump strings,
candidate counts, or final assembly text are accepted as semantic authority.
They remain forbidden authority sources by rule, but this packet does not
claim a proven current diagnostic-as-authority bug.

### Deferred Support-As-Authority Risks

The following are deferred risks, not proven bugs in this packet:

- Pointer-value indirect memory access helpers currently prove base/range
  address legality through `prepared_pointer_value_memory_has_proven_authority(...)`.
  The helper name includes "authority", but Step 3 classifies the surveyed
  fact as insufficient for pointer-value freshness because it does not select
  freshness of the named pointer at the exact load/store use
  (`src/backend/prealloc/addressing.hpp:109`).
- Global symbol memory access helpers prove exact symbol-backed memory
  address/range authority through
  `prepared_global_symbol_memory_has_publication_authority(...)`. That is
  acceptable for the exact symbol-backed memory access, but a later consumer
  must not reuse it as loaded-value, store-source, pointer-value, or global GEP
  freshness authority (`src/backend/prealloc/addressing.hpp:169`).
- Target consumers for materialization and local/global memory necessarily
  consume relocation records, memory access records, and operand shapes. This
  packet found no proof that they intentionally treat diagnostic facts as
  authority, but later implementation work should prove they do not infer
  semantic freshness from relocation-only, layout-only, range-only, or
  target-shape-only evidence.

## Implementation-Idea Test Shape

A later implementation idea should build tests around authority dimensions,
not around one target operand form:

- same value but wrong use kind must reject
- same use but wrong value/home/source object/symbol must reject
- same value and use but stale block, instruction index, or LIR coordinate
  must reject
- complete stack/local layout without selected freshness must reject freshness
- valid relocation without semantic address/freshness authority must reject
  semantic use
- valid range without matching identity, derivation coordinate, and use must
  reject semantic authority
- encodable target operand shape without prepared authority must reject support
- diagnostics may explain any of the above rejections, but cannot convert them
  into support
