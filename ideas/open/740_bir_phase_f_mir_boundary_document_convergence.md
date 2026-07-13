# BIR Phase F MIR Boundary Document Convergence

Status: Open
Type: Documentation-only architecture convergence
Phase Owner: F — strict MIR construction and emission boundary
Predecessor: accepted `ideas/open/739_bir_phase_e_allocation_document_convergence.md`
Successor: terminal assembly/object/link consumers; then umbrella final audit

## Goal

Converge the external boundary that maps each accepted allocated pseudo node to
exactly one machine record, verifies the private machine graph, and emits
assembly/object/link outputs without hiding lowering, allocation, spilling,
frame construction, or pressure repair in MIR.

## Why This Exists

The terminal boundary is only safe if machine construction applies accepted
upstream decisions exactly once and every verifier/emission owner and terminal
consumer is explicit.

## Scope and Exact Owner Order

1. F1: the directly linked external `src/backend/mir/README.md`, consuming only
   `MirReadyBirView`, exact immutable `FrameRealizationPlan`, explicit E4 frame
   actions and exact target mapping.
2. F2: audit the external MIR/target verifier authority. The root links no
   dedicated verifier Markdown owner, so classify the exact current owner or
   create/index a documentation-only boundary placeholder; do not invent
   implementation or assign it to BIR verification.
3. F3: the directly linked `src/backend/mir/object/README.md` plus explicit
   classification of target assembler, encoder, relocation, object and linker
   Markdown authorities/placeholders. Target-family documents are subordinate
   evidence, not permission to absorb target implementation into this child.
4. Classify all other unlinked MIR Markdown as authoritative F1/F2/F3 boundary
   detail, observational/support/audit material, legacy implementation truth,
   or out of this BIR-facing convergence. Every included file has one owner;
   every excluded file has a reason.

## Uniform Contract and Matrix Method

Apply the umbrella metadata/core-first format and exhaustive input/output
matrices to the two linked owners and every classified normative boundary
owner/placeholder. Each row names exact revision/target/frame/assignment/
mapping keys, stable node and record identities, optional/error forms,
producer/consumer clauses, verifier, relocation/object/link output, failure,
invalidation and checked implementation status. Support documents render or
audit authoritative facts and never become a second semantic owner.

## Boundary Obligations and Terminal Handoff

- F1 is strict one allocated pseudo node to one machine record, including
  every explicit frame action and opaque inline-asm record. It applies fixed
  homes/placements and cannot expand calls/instructions, choose ABI locations,
  create allocatable temporaries, allocate, spill, repair pressure, construct
  frame actions, or mutate the explicit graph.
- F2 verifies the private machine graph and exact mapping/product
  fingerprints. Failure returns no verified graph and cannot route backward as
  hidden repair.
- F3 alone first parses opaque inline-asm text and owns assembly/encoding,
  relocations, object formation and link consumption. Every terminal output
  has an explicit consumer and failure behavior.
- After acceptance, the downstream action is the umbrella-level final
  cross-phase Markdown inventory/adjacency audit, not implementation
  authorization.

## Non-Goals

Implementation/tests/build edits; pseudo lowering/legalization; ABI transport;
allocation/spill/frame work; redesign of target assemblers/linkers; or treating
all MIR Markdown as phase-F normative merely because of its path.

## Acceptance and Closure Criteria

- F1/F2/F3 ownership and order are exact; the two linked documents and every
  relevant unlinked authority/placeholder are explicitly classified once.
- One-node/one-record and no-repair constraints are proven by both producer
  and consumer clauses, not asserted; verifier and terminal output/failure
  matrices are complete.
- Implementation-status statements distinguish current target code, legacy
  paths, scaffolds and placeholders without claiming the accepted architecture
  already exists.
- Closure records the MIR Markdown classification, created placeholders,
  matrices, implementation truth, exact E/F adjacency, terminal consumers and
  remaining external authorities for the umbrella final audit.

## Reviewer Reject Signals

- Code/test/build edits, phase mixing, hidden MIR lowering/call expansion,
  allocation, temporary creation, spill/pressure repair or frame construction.
- One-to-one mapping accepted by assertion, F2 verifier left unowned, F3
  assembler/object/link outputs without explicit consumers, or support prose
  promoted into semantic authority.
- Heading/status-only edits, stale implementation claims, stale/mixed product
  keys, partial graph/output publication, or optional/error forms dropped.
- Named-case lowering, rendered-text tests, allowlists, unsupported downgrades,
  expectation weakening, helper renames, or classification-only changes
  claimed as progress.
- The same hidden lowering/allocation/frame failure retained under a new MIR
  abstraction name.
