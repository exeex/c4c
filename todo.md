Status: Active
Source Idea Path: ideas/open/597_pointer_address_semantic_model_research.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Position Closed Evidence And The Prepared MIR Boundary

# Current Packet

## Just Finished

Completed `plan.md` Step 5: Position Closed Evidence And The Prepared MIR
Boundary.

Filled
`docs/pointer_address_semantic_model_research/04_closed_evidence_and_mir_boundary.md`
with the closed-evidence classification and downstream MIR boundary rules.
Ideas 587 through 590 are classified as the selected freshness authority
baseline: they define use-specific selected authority with value identity,
use kind, source kind, proof kind, source reference, rank, and program point.

Ideas 592, 593, 594, and 596 are classified as narrow branch pointer
stack-source evidence for migrated RV64 fused pointer branch `Lhs` and `Rhs`
stack-slot sources. They are not global pointer/address semantic closure and
do not close pointer arithmetic, pointer-value indirect memory, semantic GEP
target consumption, relocation semantics, local-memory layout, stack-home
completeness, or target operand-shape semantics.

The document states that idea 591 may consume this research as a
`PreparedMirView` boundary contract, exposing owned semantic authorities,
support facts, target-consume facts, route proofs, diagnostic facts, and
unavailable/deferred statuses. It must avoid inventing pointer/address
freshness or address validity inside `PreparedMirView` from view shape,
complete stack homes, local layout/range facts, relocation/materialization
records, target operands, candidate counts, dumps, or diagnostics.

## Suggested Next

Step 6: Finalize Recommendations And Index.

## Watchouts

- This is a research route; do not edit implementation files, tests,
  expectations, unsupported markers, allowlists, runtime behavior, or harness
  behavior.
- Keep any Step 6 recommendations split by first owning layer with
  prerequisites, proof surface, and reviewer reject signals.
- Do not reopen the closed RV64 fused pointer branch stack-slot `Lhs`/`Rhs`
  queue or claim it proves global pointer/address semantic closure.
- Preserve deferred status for pointer base plus offset value homes,
  pointer-value indirect memory freshness, and semantic GEP target
  consumption unless Step 6 opens separate follow-up ideas.
- Treat idea 591 as a downstream consumer of this semantic model, not as the
  owner of unresolved pointer/address semantics.

## Proof

Docs/todo-only packet. Validation command:
`git diff --check` passed with no output. `test_after.log` was not updated for
this packet.
