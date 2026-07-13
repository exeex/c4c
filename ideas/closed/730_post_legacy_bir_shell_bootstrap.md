# 715-Guided BIR Core Redesign And LIR Import Migration

Status: Closed — Superseded/Retired (not completed)
Type: backend core redesign and interface migration

## Closure Note (2026-07-13)

This idea is retired as superseded, not accepted as completed.  Its bounded
foundation/bootstrap work landed through `2b6148590`, and the later
`ac2f344f2`-era active BIR provides the real core carrier and verified minimal
LIR-to-BIR interface while the legacy compile graph remains quarantined.

The monolithic migration promised here did not finish.  Unfinished families
include globals and global initializers; scalar and aggregate operations; the
memory foundation and addressing/materialization families; calls and call ABI;
remaining import analysis; and the full BIR-to-MIR migration.  Those families
are no longer ordered by this bootstrap idea.  Current ownership is expressed
by the per-directory contracts and placeholders, including
`src/backend/bir/lir_to_bir/README.md`, its `memory/README.md`, the BIR
`core/` and `verify/` contracts, the `target_layout/`, `pseudo/`, `regalloc/`,
and `allocated/` placeholders, and `src/backend/mir/README.md`.  Future work
must use narrow reviewed ideas against those current contracts.

The historical unaccepted globals patch remains in `stash@{0}` as
`wip step 6.1 globals before inline asm priority switch`.  It must not be
applied, dropped, or treated as accepted progress without a fresh reviewed
idea that checks it against the current BIR schema.  This closure does not
modify that stash and does not transfer these unfinished families into active
idea 731.

## Lifecycle Progress

Step 1 completed at `793eeeb90`: obsolete prepared-BIR, prealloc, route,
semantic-BIR, and `backend_lir_to_bir_notes` registrations were removed; CMake
generation succeeds without `src/backend/legacy/**` compile entries.  The first
production seam is the missing active `src/backend/bir/bir.hpp` included by
`src/backend/backend.hpp`.

## Historical Paused State (2026-07-13)

At this checkpoint the idea remained open but was no longer the active plan.
Steps 1--5 had completed through `2b6148590`: the bounded new-BIR foundation
and minimal verified LIR-to-BIR import were active, legacy/prealloc/MIR sources
remained quarantined, and the selected broader proof was green.  Step 6.1
globals work was started but was not accepted or committed; its eight-file
working patch was preserved in `stash@{0}`
(`wip step 6.1 globals before inline asm priority switch`).  The closure note
above supersedes the former resume instruction.

## Intent

Use the accepted idea-715 research contract to design a bounded real BIR core,
expose it through a new active `bir.hpp` facade, and migrate the retained
`src/backend/bir/lir_to_bir` family onto builders for that core.  The new BIR is
neither a copied legacy monolith nor an always-empty compatibility fake.

The design authorities are:

- `docs/backend/pass_ready_bir/03_pass_ready_bir_contract.md`
- `docs/backend/pass_ready_bir/05_target_schema_and_api_blueprint.md`

They provide ownership and API constraints, not permission to implement the
entire P0--P13 roadmap in this bootstrap idea.

## Core Foundation Contract

- `bir.hpp` is a narrow public facade over a real schema under
  `src/backend/bir/core/`; it is not the ownership location for a monolith.
- Stable semantic IDs include generation and owner information:
  `FunctionId`, `BlockId`, `InstId`, and `ValueId`.  Add module/global/local IDs
  only when an actual migrated family requires them.
- Storage ownership is separate from semantic iteration order.  Module owns
  functions and module entities; functions own blocks, instructions, values,
  parameters, and needed locals; explicit ID order lists define traversal.
- The initial semantic node set is `Value`, `Inst`, `Block`, `Function`, and
  `Module`, with only the opcode/attribute/type surface required by migrated
  families.
- A block terminator is the sole authority for CFG successors.  No route,
  predecessor, prepared, or target side table may become competing authority.
- Construction is builder-only.  Builders reject foreign owners and invalid
  references at the earliest supported stage.
- Publication returns a move-only `RawBir` result/wrapper around owned core
  storage and a read-only view.  Published core contains no route, prealloc,
  prepared, target, printer, dump, or debug authority.

## Staged Verification Contract

Verification grows with the migrated surface:

1. Foundation publication checks nonzero owner/generation IDs, live resolution,
   unique ownership, order-list membership, and required terminators.
2. Each semantic-family migration adds its operand/result/type/attribute and
   ownership rules before that family is considered published.
3. CFG-bearing publication verifies that successors derive only from legal
   terminators and remain within the owning function.
4. Raw publication rejects unresolved builder tokens and any forbidden legacy,
   route, prealloc, prepared, target, printer, or debug authority.

Full editor transactions, RAUW, analysis managers, canonical pass pipelines,
and the full verifier roadmap remain later idea-715 follow-ups unless a concrete
LIR import packet proves a minimal piece is necessary for its direct contract.

## LIR-To-BIR Migration Order

Inventory and migrate the existing family exactly in this order, refining a
packet only when build evidence shows an internal dependency:

1. import spine: `lowering.hpp`, `context.cpp`, `module.cpp`, `types.cpp`
2. CFG publication: `cfg.cpp`
3. globals: `globals.cpp`, `global_initializers.cpp`
4. scalar and aggregate: `scalar.cpp`, `aggregate.cpp`
5. memory foundation: `memory/memory_types.hpp`,
   `memory/memory_helpers.hpp`, `memory/local_slots.cpp`,
   `memory/addressing.cpp`, `memory/provenance.cpp`,
   `memory/value_materialization.cpp`, `memory/local_gep.cpp`,
   `memory/intrinsics.cpp`, `memory/coordinator.cpp`
6. calls: `calling.cpp`, `call_abi.cpp`
7. remaining import analysis: `analysis.cpp`

Each family constructs through the new builders, adds its matching verification
rules, and receives build plus direct LIR-to-new-BIR proof before the next
family begins.  Not-yet-migrated forms must return a clear safe rejection; they
must not silently disappear and must not fall back to legacy BIR.

## BIR-To-MIR Boundary

The initial BIR-to-MIR consumer may accept only the newly published Raw/verified
view and the semantic subset explicitly migrated so far.  Unsupported non-empty
semantics reject safely and observably.  This idea does not revive prealloc,
prepared BIR, target planning, or legacy lowering authority.

## Test Policy

Retain or add only direct LIR-to-new-BIR and new-BIR-to-MIR tests.  Stable-ID,
owner/generation, order, and terminator-authority behavior belongs in durable
tests only when observable through one of those interfaces.  Packet-local
internal proof may be transient.  Do not preserve internal route/prealloc,
printer/dump/lookup/ID-shape, target, object, or runtime tests.

## In Scope

- A reviewed schema/API checkpoint before broad implementation.
- Real bounded BIR IDs, storage/order, semantic nodes, builders, RawBir
  publication, read-only views, and staged verification.
- A narrow `bir.hpp` facade over the core.
- Ordered migration of every retained `lir_to_bir` family listed above.
- A bounded Raw/verified BIR-to-MIR consumer with safe unsupported rejection.
- Direct two-interface tests and final default build plus broader CTest.

## Out Of Scope

- Compiling, copying, renaming, wrapping, or re-exporting
  `src/backend/legacy/**`.
- Implementing all P0--P13 work now.
- FunctionEditor, RAUW, analysis manager, general CFG mutation, canonical pass
  suite, or full preparation pipeline without a demonstrated migration need.
- Reviving prealloc/prepared or target realization authority in core BIR.
- Silent empty output for meaningful not-yet-migrated LIR.

## Acceptance Criteria

- A schema/API checkpoint records the bounded file/type/API surface and maps it
  to the two design-authority documents before broad implementation.
- New `bir.hpp` exposes a real core facade with owner/generation-aware IDs,
  separate ownership/order, semantic nodes, terminator-only successors,
  builder-only construction, RawBir publication, and staged verification.
- Published core contains no legacy, route, prealloc, prepared, target,
  printer, dump, or debug authority.
- Every listed `lir_to_bir` file is migrated in the declared family order or is
  explicitly rejected with remaining work recorded; accepted families have
  direct interface proof and build proof.
- Not-yet-supported forms reject safely without legacy fallback or silent loss.
- BIR-to-MIR consumes only the supported published Raw/verified view and rejects
  unsupported semantics without prealloc revival.
- Build metadata contains no `src/backend/legacy/**` compile entry.
- Direct LIR-to-new-BIR and new-BIR-to-MIR tests are non-empty and green; the
  final default build and broader CTest are reviewed.
- Editor/RAUW/analysis/canonical-pass work not required by migration is recorded
  as later 715 follow-up scope rather than smuggled into this implementation.

## Reviewer Reject Signals

- `bir.hpp` becomes a copied or renamed legacy monolith, an always-empty fake,
  or a public mutable container surface.
- IDs are vector positions, pointers, names, labels, or unowned integers; slot
  reuse can resolve a stale ID to a new entity; allocation order defines
  traversal order.
- CFG successors exist outside terminators as authoritative route/predecessor
  state.
- Published RawBir contains route, prealloc, prepared, target, printer, dump,
  debug, or legacy compatibility authority.
- Broad P0--P13 infrastructure lands before the bounded schema checkpoint or
  without a concrete migration need.
- `lir_to_bir` is made to compile through legacy includes/API transplantation,
  silent dropping, family-shaped shortcuts, or migration without matching
  verification and direct-interface proof.
- Families are migrated opportunistically without the declared inventory/order
  or without recording safe rejection for remaining forms.
- BIR-to-MIR revives prealloc/prepared state or pretends unsupported non-empty
  semantics reached MIR successfully.
- Internal-only tests are retained, or stable-ID/CFG tests are claimed as
  durable protection without observation through one retained interface.
