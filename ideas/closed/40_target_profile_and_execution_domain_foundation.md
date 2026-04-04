# Target Profile and Execution-Domain Foundation

Status: Closed
Last Updated: 2026-04-04

## Goal

Introduce a first-class target-profile model and declaration-level
execution-domain tracking so frontend-to-backend lowering has one authoritative
source of target/ABI/data-layout truth, and future host/device multi-target
codegen can split cleanly into per-target LIR/LLVM/backend outputs.

## Why This Comes Before Idea 41

The current backend migration work is converging too much test coverage onto
the RISC-V passthrough route because that path is the easiest place to observe
"did we stay on direct BIR or fall back to LLVM IR text?".

That is useful as a narrow route oracle, but it also exposes a deeper
architectural gap:

- frontend and backend currently pass around a raw `target_triple` string
- `data_layout` is derived later, mostly in `HIR -> LIR`
- target ABI traits are queried ad hoc from triple/layout helpers
- there is no first-class execution-domain model for future `__host__` /
  `__device__` style splitting
- the current pipeline still assumes one translation unit lowers to one LIR
  module

If idea 41 continues before this foundation is cleaned up, we risk baking more
backend behavior and more tests into an architecture that cannot scale cleanly
to multi-target compilation or explicit execution-domain routing.

## Current Architecture Gap

Today the target-related information is fragmented:

- CLI chooses `--target <triple>`
- preprocessor consumes the triple for predefined macros
- sema/HIR retain the triple
- `HIR -> LIR` derives `data_layout` from the triple if absent
- backend re-parses the triple into `backend::Target`
- target ABI details are mostly exposed through helper functions, not through a
  single shared target object

This is workable for single-target host-only codegen, but it is not a stable
base for:

- explicit host/device declaration domains
- a future `HostDevice` surface
- generating more than one backend module from a single source file
- making target-sensitive tests converge on architecture-neutral contracts

## Desired End State

### 1. First-class `TargetProfile`

Create a shared `TargetProfile` model that owns:

- canonical `target_triple`
- canonical `data_layout`
- backend target enum / object-format family
- ABI-relevant traits such as pointer model, aggregate passing model,
  `va_list` representation, long-double model, and relevant address-space
  defaults

Codegen should ask the profile for traits instead of repeatedly re-deriving
them from strings.

### 2. First-class `ExecutionDomain`

Introduce declaration-level execution-domain metadata, initially:

- `Host`
- `Device`
- `HostDevice`

This is not a lexical block scope in the IR. It is a property recorded on each
function/global declaration after frontend state (attribute or pragma-driven)
is resolved.

Frontend pragmas such as a future:

- `#pragma c4 exec host`
- `#pragma c4 exec device`

should be treated only as input syntax for changing the default declaration
domain of following declarations. They are not the long-term semantic model.

The durable semantic model should live on HIR declarations themselves, so later
lowering does not need to reconstruct execution domain from pragma history.

### 3. Multi-target lowering seam at `HIR -> LIR`

Keep HIR aware of execution domain, then let the lowering coordinator split one
HIR module into one or more target-specific LIR modules:

- host LIR contains `Host` + `HostDevice`
- device LIR contains `Device` + `HostDevice`

In other words, the desired future model is not "wrap the whole file in an
implicit `#pragma host on/off` block and lower one monolithic module". The
desired model is "tag each relevant HIR declaration with its execution domain,
then partition declarations into one or more target-specific LIR products".

The current implementation may stay host-only for now, but the lowering API and
data model should stop assuming "one input source == one output LIR module
forever".

### 4. Module header ownership of target/ABI

Per-target module products should carry their target profile explicitly near the
module root:

- target triple
- data layout
- any future per-target ABI metadata needed by lowering/emission

## Scope

This idea covers the architectural foundation only:

- introducing `TargetProfile`
- threading it through frontend/codegen boundaries
- introducing `ExecutionDomain` metadata in AST/HIR-facing lowering surfaces
- defining the future split point for multi-target `HIR -> LIR`
- updating tests so they rely less on hard-coded per-file triples/layouts

## Non-Goals

- no full `__device__` implementation yet
- no GPU/backend bring-up yet
- no cross-domain linker/runtime implementation yet
- no forced rewrite of idea 41’s BIR work beyond what is necessary to stop
  coupling it to the old target-string architecture

## Proposed Phases

### Phase 1: Inventory target/ABI ownership

- inventory every place that stores or derives `target_triple`
- inventory every place that stores or derives `data_layout`
- inventory helper functions that should become `TargetProfile` trait queries
- inventory tests that hard-code per-file target/layout literals

### Phase 2: Introduce `TargetProfile`

- add a shared `TargetProfile` type and canonical construction path
- create one resolver from CLI triple to canonical profile
- thread `TargetProfile` through preprocessor/sema/HIR/LIR/codegen entry points
- stop re-parsing target identity independently at each layer where practical

### Phase 3: Introduce `ExecutionDomain`

- add declaration-level execution-domain metadata in frontend/HIR structures
- keep default behavior as `Host` everywhere
- define the semantic model now, even if parsing initially only supports the
  default host-only path
- make it explicit that pragma state is only a frontend convenience for
  assigning declaration tags; pragma boundaries themselves do not become IR
  scope objects
- identify the minimum HIR nodes that must carry the tag directly, starting with
  functions and globals

### Phase 4: Prepare `HIR -> LIR` for split lowering

- separate "build one target-specific LIR module" from "choose which target
  modules to emit"
- keep current output behavior host-only, but make the lowering boundary able to
  return multiple target-specific products later
- ensure LIR and backend entry points use target-profile-owned module metadata
- define the future partition rule explicitly:
  - `Host` declarations feed `host.ll`
  - `Device` declarations feed `device.ll`
  - `HostDevice` declarations can appear in both products subject to later
    target-specific legality rules

### Phase 5: Realign tests

- centralize target/layout fixtures in backend/frontend tests
- reduce accidental dependence on RISC-V-only passthrough tests as the main BIR
  route oracle
- clarify which tests validate target-neutral lowering contracts vs
  target-specific emission contracts

## Acceptance Criteria

- [ ] there is a shared `TargetProfile` type used across frontend/codegen
- [ ] `data_layout` comes from canonical target-profile ownership rather than
      scattered ad hoc derivation
- [ ] HIR retains declaration-level `ExecutionDomain` metadata
- [ ] `HIR -> LIR` lowering no longer assumes the architecture can never split
      into multiple target products
- [ ] backend/frontend tests have centralized target/layout fixtures for the
      active single-target path
- [ ] idea 41 can resume on top of the new target-profile foundation without
      leaning on RISC-V-only route tests as the de facto architectural oracle

## Follow-on Relationship

This idea intentionally precedes and de-risks:

- `ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md`

Once this foundation is in place, idea 41 should resume with cleaner ownership
boundaries between target-neutral backend IR, target profile, and
target-specific emission.

## Completion Note

Closed on 2026-04-04 after the foundation slice reached the point needed to
unblock idea 41:

- `ExecutionDomain` metadata now exists on declaration/HIR surfaces with the
  current host-only/default-host path intact
- `c4cll` has a minimal split-LLVM path with separate host/device target
  triples and default paired outputs (`xxx.ll`, `xxx.device.ll`)
- backend test ownership started moving away from one mixed target bucket
  toward explicit per-platform test files and centralized target-profile
  helpers

Remaining follow-through that builds on this foundation is intentionally left
to:

- `ideas/open/41_bir_full_coverage_and_ir_legacy_removal.md`
- `ideas/open/42_typed_lir_type_ref_and_bir_ownership_follow_through.md`
