# 02. Semantic Authority And Fact Classes

Status: Step 3 complete

## Purpose

This file classifies the pointer/address families inventoried in
`01_pointer_address_family_inventory.md`. The classification separates the
fact that authorizes a pointer/address use from facts that only support,
prove, lower, or diagnose that use.

Role names:

- Semantic authority: required to decide whether a pointer/address value is
  current for a specific use.
- Verifier/support fact: proves route coherence, identity, range, ordering, or
  layout, but cannot authorize the use by itself.
- Target-consume fact: needed by a backend after semantic authority has
  already been established.
- Route proof: supports diagnostics, review, or debugging.
- Diagnostic-only artifact: observational output that must not become
  semantic authority.

## Global Authority Rule

A pointer/address value is current for a use only when the owning semantic
layer names the same value, the same use kind, the same program point or
derivation coordinate, and the proof that makes that use valid.

The following facts are never sufficient authority by themselves:

- Stack-home completeness. A complete `PreparedValueHomeKind::StackSlot`,
  frame-slot id, offset, size, alignment, or stack object can identify a home,
  but closed branch freshness work explicitly rejects stack-home-only
  authority (`ideas/closed/590_branch_stack_load_freshness_contract.md`,
  `ideas/closed/593_rv64_branch_stack_source_freshness_consumption.md`).
- Local-memory layout. `PreparedMemoryAccess`, `PreparedAddress`, range,
  extent, and layout authority prove address legality for a memory use, but
  not that the pointer value or loaded/stored value is fresh
  (`src/backend/prealloc/addressing.hpp:95`,
  `src/backend/prealloc/addressing.hpp:109`,
  `src/backend/prealloc/addressing.hpp:169`).
- Target operand shape. `ResolvedOperand`, target memory operands, register
  numbers, final operand text, and object-emission fragments are target inputs
  or diagnostics, not semantic authority (`src/backend/mir/aarch64/codegen/operands.cpp:88`,
  `src/backend/mir/aarch64/codegen/operands.cpp:151`).
- Relocation materialization. `PreparedAddressMaterialization` tells a target
  how to materialize a frame, symbol, TLS, or fixup address; it does not prove
  freshness of the pointer value at a later use
  (`src/backend/prealloc/addressing.hpp:323`,
  `src/backend/prealloc/stack_layout/coordinator.cpp:1445`).

## Classification Summary

| Family | Authority decision |
| --- | --- |
| Pointer base plus offset value homes | Deferred: home shape is support/target evidence until a prepared pointer-arithmetic authority names the base freshness, result identity, delta, and use. |
| BIR pointer arithmetic materialization for frame addresses | No freshness authority: frame-slot materialization is verifier/support and target-consume evidence. |
| Semantic relocation and global address materialization | No freshness authority: relocation/materialization is target-consume evidence plus symbol-policy support. |
| Global symbol memory accesses | Semantic address-access authority for exact symbol-backed memory accesses; not pointer-value freshness authority. |
| Local stack or frame-slot addressing | Verifier/support and target-consume only unless paired with separate value freshness authority. |
| Pointer-value indirect memory accesses | Deferred: current helper proves address legality/range, but base pointer freshness needs a first owner and proof surface. |
| Local-array source object and address derivation | Route proof and verifier/support; authority lives in the semantic GEP availability record, not raw derivation artifacts. |
| Local-array semantic GEP availability | Semantic authority for local-array address derivation when the semantic GEP record is `Available`. |
| Global static semantic GEP | Semantic authority for global static GEP address derivation when the semantic GEP record is `Available`; target consumption remains separate. |
| Branch pointer stack-source operands | Semantic freshness authority for the exact branch stack-load source use, and only for that narrow use. |
| Target-local operand shape | Target-consume and diagnostic-only; never semantic authority. |

## Family Classifications

### Pointer Base Plus Offset Value Homes

- Semantic authority rule: deferred. A
  `PreparedValueHomeKind::PointerBasePlusOffset` home can participate in an
  authorized use only after a separate semantic authority proves the base
  pointer is fresh for that use, the result value identity is the intended
  pointer result, and the byte delta is the semantic derivation consumed by
  that use (`src/backend/prealloc/value_locations.hpp:19`,
  `src/backend/prealloc/value_locations.hpp:320`).
- Support facts: base value name, optional base symbol name, byte delta, value
  home id, and any route that proves the base/result relationship.
- Target-consume facts: target-encodable offset shape, stack/register
  placement, and backend lowering choices such as RV64 local-memory offset
  checks or AArch64 rejection of computed/pointer-base-plus-offset decoded
  homes (`src/backend/mir/aarch64/codegen/operands.cpp:151`).
- Route proofs: prepared dumps or lookups that expose the normalized
  pointer-base-plus-offset fact.
- Diagnostic-only facts: unsupported target diagnostics for computed address
  homes.
- Deferred gaps: missing first owner is the shared prepared/prealloc
  pointer-arithmetic publication layer. Missing proof surface is a selected
  authority that links base freshness, result pointer identity, delta, use,
  and program point while rejecting stale-base and wrong-use cases.

### BIR Pointer Arithmetic Materialization For Frame Addresses

- Semantic authority rule: not a freshness authority. Frame-slot address
  materialization authorizes only the materialization route for a known frame
  address shape; it does not prove the pointed-to value or result pointer is
  current for a later semantic use
  (`src/backend/prealloc/stack_layout/coordinator.cpp:1377`).
- Support facts: pointer-typed BIR binary instruction, frame-address base
  value, immediate byte delta, frame-slot id, and final offset.
- Target-consume facts: AArch64 `AddressMaterializationRecord`, RV64 indexed
  materialization search, and x86 frame-slot operand rendering consume the
  materialized shape after earlier facts select the route.
- Route proofs: `PreparedAddressMaterializationKind::FrameSlot` records.
- Diagnostic-only facts: notes or dumps that show a materialization record was
  or was not found.
- Deferred gaps: none for this packet. It is deliberately classified as
  support/target evidence, not semantic pointer freshness.

### Semantic Relocation And Global Address Materialization

- Semantic authority rule: not pointer freshness authority. Structured symbol
  identity, materialization policy, TLS model, and relocation fields authorize
  target materialization of a symbol address, but they do not prove that a
  pointer value is current at an arbitrary consuming use
  (`src/backend/prealloc/stack_layout/coordinator.cpp:1445`,
  `src/backend/prealloc/addressing.hpp:323`).
- Support facts: `LinkNameId`, global/function identity, materialization
  policy, address space, TLS flags, and offset.
- Target-consume facts: direct, GOT, TLS, string-constant, object-fixup, and
  target relocation records.
- Route proofs: missing-materialization notes and prepared address
  materialization dumps.
- Diagnostic-only facts: target unsupported messages for missing structured
  symbol spelling, unknown symbol, or unsupported relocation route.
- Deferred gaps: none for relocation itself. Any later pointer freshness
  claim must be owned by a selected value freshness or semantic address
  derivation fact, not by the relocation record.

### Global Symbol Memory Accesses

- Semantic authority rule: a prepared global symbol memory access can
  authorize the exact symbol-backed memory access when
  `prepared_global_symbol_memory_has_publication_authority(...)` accepts the
  same `PreparedAddress`: base kind is `GlobalSymbol`, symbol identity is
  present, layout authority is not unknown/opaque, object extent is complete,
  and requested range exactly matches the access and is proven in bounds
  (`src/backend/prealloc/addressing.hpp:169`). This authority is for the
  memory address/range use, not for freshness of an independently carried
  pointer value or loaded/stored scalar.
- Support facts: `PreparedMemoryAccess`, provenance base identity, layout
  authority, size, alignment, volatility, address space, and in-bounds range
  (`src/backend/prealloc/addressing.hpp:348`).
- Target-consume facts: AArch64 memory records, RV64 global memory support
  checks, and x86 same-module global operand rendering.
- Route proofs: prepared memory access rows and global publication authority
  helper outcomes.
- Diagnostic-only facts: target diagnostics for unsupported policy, range,
  or operand form.
- Deferred gaps: no missing first owner for exact symbol-backed memory access.
  Loaded-value freshness, store-source freshness, and pointer-value freshness
  remain separate freshness-authority questions.

### Local Stack Or Frame-Slot Addressing

- Semantic authority rule: frame-slot access facts prove a local load/store
  address route, but they do not prove value freshness. A stack or frame-slot
  address is usable for target local memory only when the prepared access names
  the exact frame slot, offset, size, alignment, provenance, and instruction
  use; a branch, call, publication, or move source still needs its own
  selected freshness authority before treating the stack value as current
  (`src/backend/prealloc/addressing.hpp:95`,
  `ideas/closed/590_branch_stack_load_freshness_contract.md`).
- Support facts: frame-slot id, stack object, offset, size, alignment,
  address provenance, and layout/range checks.
- Target-consume facts: AArch64 frame-slot records, RV64 local-memory
  emission, and x86 frame-slot operand rendering.
- Route proofs: prepared local memory access rows and frame-slot helper
  status.
- Diagnostic-only facts: missing frame slot, layout, clobber, or unsupported
  local operand diagnostics.
- Deferred gaps: value freshness for stack-homed sources is owned by
  use-specific freshness routes such as branch stack-load source freshness,
  direct edge-publication source freshness, or move-bundle source freshness.

### Pointer-Value Indirect Memory Accesses

- Semantic authority rule: deferred for current pointer value freshness.
  `prepared_pointer_value_memory_has_proven_authority(...)` proves important
  address legality for an exact pointer-value memory access: base kind is
  `PointerValue`, the pointer value name exists, base-plus-offset is allowed,
  provenance base identity is not unknown/pointer-only, layout authority is
  accepted, extent is complete, and the requested range exactly matches a
  proven in-bounds access (`src/backend/prealloc/addressing.hpp:109`). That
  still does not prove the named pointer value is fresh at the load/store use.
- Support facts: pointer value name, offset, range, object extent, layout
  authority, access size/alignment, and provenance.
- Target-consume facts: target memory operands and offset encodability for
  scalar pointer-value load/store lowering.
- Route proofs: prepared memory access rows, provenance/range statuses, and
  publication-planning source-memory facts.
- Diagnostic-only facts: unsupported pointer-value memory diagnostics and
  dumps.
- Deferred gaps: missing first owner is a shared prepared/prealloc
  pointer-value memory-use freshness publisher. Missing proof surface is a
  selected authority that pairs the named pointer value with the exact
  load/store use and rejects stale, wrong-value, wrong-use, and range-only
  evidence.

### Local-Array Source Object And Address Derivation

- Semantic authority rule: raw local-array source-object, derivation,
  element-path, checker-input, and range-proof records are not authority by
  themselves. They are the route facts that the semantic GEP evaluator must
  consume before an address derivation can become available
  (`src/backend/bir/bir_local_array_semantic_gep.hpp:2453`).
- Support facts: alloca source object, derivation kind, LIR producer
  coordinate, element path, dynamic index identity, checker input, and range
  proof.
- Target-consume facts: none directly. Targets should consume later prepared
  memory/access or materialization facts, not raw local-array proof records.
- Route proofs: local-array derivation records, interval effects,
  range-proof records, checker inputs, local-address provenance, and scalar
  local-load records.
- Diagnostic-only facts: local-array status dumps and carrier diagnostics.
- Deferred gaps: none for raw derivation records because they are
  intentionally support/proof. Authority is classified in the next family.

### Local-Array Semantic GEP Availability

- Semantic authority rule: `LocalArraySemanticGepRecord` with status
  `Available` is semantic authority for the selected local-array address
  derivation. The evaluator requires source object availability, matching
  derivation and element path, an address-derivation LIR coordinate, single
  dynamic index identity, available range proof, scalar element shape, and
  in-bounds byte range before setting `Available`
  (`src/backend/bir/bir_local_array_semantic_gep.hpp:2575`).
- Support facts: local-array provenance, range checker status, source object,
  derivation, element path, dynamic index, element type/size/count, byte
  offset, and source total size.
- Target-consume facts: none directly in the semantic GEP record. Later
  target lowering still needs memory access or materialization facts.
- Route proofs: provenance and semantic GEP dumps.
- Diagnostic-only facts: local-array carrier status names and debug output.
- Deferred gaps: target-consumer migration is deferred; first owner would be
  the later prepared/MIR view or target-consumption route. The semantic
  address-derivation authority itself is decided here.

### Global Static Semantic GEP

- Semantic authority rule: `GlobalStaticSemanticGepRecord` with status
  `Available` is semantic authority for the selected global static GEP address
  derivation. The evaluator requires a global source object, link-name
  identity, source layout, derived pointer identity, layout path, element byte
  range, constant or dynamic range authority, proven in-bounds range,
  non-pointer element boundary, and available LIR producer coordinate
  (`src/backend/bir/bir_local_array_semantic_gep.hpp:2863`).
- Support facts: global static GEP authority record, global identity,
  layout/range authority, derived pointer name, element metadata, and LIR
  producer coordinate.
- Target-consume facts: targets currently still consume prepared address
  materialization or prepared memory access facts for actual emission.
- Route proofs: semantic GEP records and prepared global derivation evidence.
- Diagnostic-only facts: global static GEP status dumps.
- Deferred gaps: target-consumer migration is deferred. Missing proof surface
  is a later consumer proof that MIR/target code reads the semantic GEP
  authority rather than inferring validity from relocation or operand shape.

### Branch Pointer Stack-Source Operands

- Semantic authority rule: selected branch stack-load source freshness is
  authority only for the exact branch stack-load source use. The selected fact
  must match the same prepared value/home, use kind
  `PreparedValueFreshnessUseKind::BranchStackLoadSource`, source kind
  `PreparedValueFreshnessSourceKind::BranchStackSlot`, proof kind
  `PreparedValueFreshnessProofKind::BranchTerminatorOrdering`, rank
  `PreparedValueFreshnessSourceRank::BranchStackSlot`, and exact branch block
  plus terminator instruction point (`src/backend/prealloc/value_locations.hpp:75`,
  `src/backend/prealloc/value_locations.hpp:106`,
  `src/backend/prealloc/value_locations.hpp:143`,
  `src/backend/prealloc/value_locations.hpp:180`,
  `src/backend/mir/riscv/codegen/object_emission.cpp:9446`).
- Support facts: branch condition record, value home, frame slot, stack
  object, clobber safety, pointer status, branch role, and control-flow target
  match (`src/backend/prealloc/publication_plans.cpp:2863`).
- Target-consume facts: RV64 fused pointer branch emission consumes the
  selected freshness result before moving operands into scratch registers and
  emitting the branch (`src/backend/mir/riscv/codegen/object_emission.cpp:9737`).
- Route proofs: closed ideas 590, 592, 593, 594, and 596 plus prepared dump
  rows exposing selected/missing freshness.
- Diagnostic-only facts: RV64 unsupported branch stack-load freshness
  diagnostic strings and candidate counts
  (`src/backend/mir/riscv/codegen/object_emission.cpp:9680`).
- Deferred gaps: no deferred gap for the migrated RV64 pointer `Lhs`/`Rhs`
  fused-branch stack-source use. The authority must not be generalized to
  aggregate-adjacent branches, select/edge publication, local-array,
  relocation, or generic pointer arithmetic.

### Target-Local Operand Shape

- Semantic authority rule: none. Target-local operand shape is produced after
  prepared/prealloc facts have already selected or rejected a route. It cannot
  authorize pointer freshness, address validity, or semantic derivation by
  itself (`src/backend/mir/aarch64/codegen/operands.cpp:88`,
  `src/backend/mir/aarch64/codegen/operands.cpp:151`).
- Support facts: target operand authority enums can preserve where a final
  operand came from for review, but they do not decide semantic validity.
- Target-consume facts: final registers, frame slots, symbols, immediates,
  memory operands, fixups, and emitted fragments.
- Route proofs: target lowering traces and object-emission route checks.
- Diagnostic-only facts: printed operands, assembly text, and unsupported
  target-shape diagnostics.
- Deferred gaps: none. If a later target path needs semantic authority, it
  must consume a prepared semantic/freshness authority before forming or
  printing the operand.

## Deferred Families

Deferred families and missing first owners:

- Pointer base plus offset value homes: shared prepared/prealloc
  pointer-arithmetic publication must own a selected authority for base
  freshness, result identity, delta, use, and program point.
- Pointer-value indirect memory accesses: shared prepared/prealloc
  pointer-value memory-use freshness must own a selected authority for the
  named pointer value at the exact load/store use.
- Local-array and global static semantic GEP target consumption: semantic
  address derivation is classified, but a later MIR/target route must prove it
  consumes that authority instead of relocation, memory layout, or target
  shape.

These are deferred because their first owner or consumer proof surface is not
settled by Step 3. They should be used by Step 4 to define fail-closed rules
instead of being silently treated as accepted target behavior.
