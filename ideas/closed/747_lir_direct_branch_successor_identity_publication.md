# LIR Direct-Branch Successor Identity Publication

Status: Closed (capability complete; producer handoff accepted by idea 734)
Type: bounded LIR producer-authority repair
Blocked Consumer: `ideas/open/734_lir_to_new_bir_container_completeness.md`

## Goal

Publish one direct unconditional `LirBr` successor as its existing structural
`LirBlockId`, so a receiver can preserve one CFG edge without deriving it from
a target-label spelling.

## Why This Exists

Idea 734 accepted its selected ordinary scalar and i32/i64 inline-assembly
receipts through Step 6.2, but its next terminator row cannot be received
losslessly: the authority matrix records `LirBr` with a raw target label while
the existing `LirBlockId` is already assigned structurally from block
ownership. The consumer forbids text recovery and does not own LIR schema or
producer edits beyond its narrowly stated inline-assembly exception.

## In Scope

- Add one authoritative current-function `LirBlockId` successor carrier to
  the exact direct unconditional `LirBr` producer path while retaining its
  compatibility target spelling only as presentation.
- Make the LIR verifier require that the carried successor identifies exactly
  one block in the same function and agrees with the emitted direct branch.
- Add nearby producer/verifier proof for valid, missing, invalid, and
  cross-function direct-successor identity, without accepting label-text
  recovery.
- Publish a compact handoff naming the exact `LirBr` field, ownership rule,
  focused proof, and fail-closed neighbor boundary for idea 734.

## Out Of Scope

- Raw-BIR containers, importer wiring, Raw-BIR verification, canonical BIR,
  target interpretation, branch lowering, or receiver implementation.
- Conditional branch, switch, indirect branch, phi predecessor, block
  arguments, local/object, or inline-assembly authority changes.
- Deriving a successor identity from rendered labels, printer output, test
  names, or an additional parallel label/ID table.

## Acceptance Criteria

- The direct `LirBr` path carries one same-function structural `LirBlockId`;
  its compatibility label is not used as semantic CFG authority.
- The LIR verifier rejects missing, invalid, and cross-function successor IDs.
- Focused producer/verifier evidence covers the direct branch plus neighboring
  malformed authority and shows no text-based fallback.
- A handoff lets idea 734 select one direct-branch Raw-BIR receiver packet
  without re-auditing producer authority.

## Closure Record

Close accepted: capability complete.

- Commit `cebc0a3bf` publishes `LirBr.successor` as the sole structural,
  same-function `LirBlockId` authority while retaining `target_label` only as
  display parity.
- The durable receiver handoff is
  `docs/lir_structured_identity/handoff_to_734.md`, including missing,
  invalid, cross-function, duplicate-ownership, and display-mismatch
  fail-closed boundaries.
- Accepted focused proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`.
- Idea 734 resumes at Step 6.3 with exactly one Raw-BIR direct-jump receiver
  packet. It must map `LirBr.successor` directly, verify same-function edge
  ownership and transactionality, and must not recover an edge from
  `target_label`. Conditional, switch, indirect, and phi work remain outside
  this completed producer blocker.

## Reviewer Reject Signals

- Reject a name/label lookup, printer parse, or testcase-specific mapping
  claimed as successor identity.
- Reject widening to conditional, switch, indirect, phi, or general CFG work.
- Reject Raw-BIR/importer changes claimed as completion of this LIR producer
  authority blocker.
- Reject expectation downgrades, helper-only changes, or coverage that omits
  missing/invalid/cross-function successor rejection.
- Reject a parallel side table or duplicate authority that leaves `LirBr`
  itself raw and forces its consumer to recover the edge indirectly.
