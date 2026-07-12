# Deterministic Proof and Follow-Up Boundary

## Outcome

A legitimate deterministic positive exists only if the follow-up preserves and
enforces **explicit-register inline-assembly operand constraints** as prepared
semantic input. Inline assembly independently requires operand placement; the
register choice is not introduced to make a phi testcase pass. The positive
can then place a predecessor-produced value and its join phi in two different
explicit registers and require a genuine edge move.

Current BIR has structured `InlineAsmOperandMetadata` for operand kind,
constraint text, class, group width, ties, and argument/output identity, but it
does not publish an explicit physical target identity into regalloc.
`src/backend/prealloc/inline_asm.cpp` already parses AArch64 and RV64 register
spellings into `PreparedTargetRegisterIdentity` for post-regalloc carrier
validation. The follow-up may reuse that target-identity logic at the earlier
prepared ingress seam; it must not invent a generic test-only constraint source.

## Route-independent positive semantic program

Use a target whose explicit inline-assembly register identity is already
recognized, preferably RV64 because
`rv64_inline_asm_register_identity` handles canonical `xN` and ABI spellings.
The semantic program shape is:

1. In predecessor `left`, an inline-assembly output defines named scalar
   `%incoming` with an explicit output constraint requiring one legal GPR,
   represented below as register A.
2. Another predecessor supplies an immediate or independently valid value so
   the join remains a real phi rather than a renamed straight-line definition.
3. Join block `join` defines `%phi = phi [%incoming, left], [...]`.
4. The first semantic use of `%phi` is an inline-assembly register input with an
   explicit constraint requiring a different legal GPR, register B.
5. Both explicit constraints have real inline-assembly meaning even if the
   phi/publication query is removed from the test. They are not allocation
   hints selected by a fixture.

The prepared ingress producer resolves the two operand constraints to
structured target identities. Common regalloc admits `%incoming` as fixed to A
and `%phi` as fixed to B, enforces both live-range constraints, and out-of-SSA
publishes the required A-to-B edge move. The acceptance query then calls
`PreparedMirFunctionView::current_block_direct_edge_publication_sources` on
`join` and observes one genuine named register-source fact as `Available`, with
matching move, publication, source/destination identities, homes, and selected
freshness authority.

The exact registers are determined by the inline-assembly source constraints,
not allocator order. The test should use two ordinary allocatable non-reserved
registers validated by the active target profile. It must assert the structured
identities first, then derive any expected diagnostic spelling from them.

## Why the semantic constraint is independent

- `src/backend/bir/bir.hpp::InlineAsmOperandMetadata` already treats operand
  constraints as semantic inline-assembly metadata, including register input,
  register output, tied input, class, and width.
- `src/backend/prealloc/inline_asm.cpp::validate_inline_asm_carrier` already
  rejects register operands without register homes, rejects incompatible
  register classes, and checks tied input/output identity equality.
- `inline_asm_register_identity`,
  `aarch64_inline_asm_register_identity`, and
  `rv64_inline_asm_register_identity` already establish target-aware structured
  identity for supported register spellings.

Those checks currently occur after regalloc homes exist, so they diagnose
rather than constrain allocation. Moving authenticated explicit-identity
publication earlier and enforcing it in common regalloc gives the same semantic
constraint causal authority. The direct-edge publication is downstream proof,
not the reason the constraint exists.

## Adjacent negative proof matrix

Each negative must inspect a typed admission/allocation status and confirm that
no assignment or direct-edge source is presented as available from incomplete
authority.

| Negative | Construction | Required result |
|---|---|---|
| Missing producer | Register-class operand metadata has no explicit-register identity where the proposed fixed route requires one | No fixed request is published; a test requiring fixed authority observes a precise missing-producer/input status |
| Missing value | Explicit constraint refers to an operand without a named BIR value | Admission rejects missing value identity; no constraint row is synthesized |
| Stale identity | Prepared request names a function/value pair absent from current liveness | Admission reports stale/missing liveness identity; allocator never sees the row |
| Ambiguous producer | Two distinct explicit fixed identities claim the same named value | Reject ambiguous fixed authority before allocation |
| Class mismatch | GPR identity is attached to float/vector operand metadata, or vice versa | Reject target/class incompatibility before candidate selection |
| Width mismatch | Explicit span cannot satisfy `register_group_width` or overlaps the end of a register bank | Reject unsupported width/span; do not truncate to width one |
| Unsupported target/register | Explicit spelling cannot map through the active target identity parser/profile | Reject unsupported target identity; do not retain raw spelling as authority |
| Provenance mismatch | Request source reference points to a different inline-assembly operand/value | Reject mismatched provenance and publish no normalized fixed row |
| Fixed/home conflict | Value both requires a home slot and has fixed-register-only authority without a defined dual-storage contract | Reject contradictory requirements rather than choosing one silently |
| Interference conflict | Two overlapping values are genuinely fixed to the same span | Deterministic allocation failure; neither is silently spilled or reassigned |
| Preferred unavailable | A separately tested preferred request names a legal occupied span | Stable fallback to remaining legal candidates; preference is not promoted to fixed |
| Forbidden exhaustion | Authenticated forbidden identities remove every legal span | Spill only if allowed; otherwise deterministic requires-register failure |
| Publication mismatch | After successful fixed allocation, edge publication or move points to different value/edge/register facts | Existing direct-edge query remains precise fail-closed mismatch/unsupported state |
| Freshness missing/ambiguous | Move/publication exists but source freshness authority is absent or duplicated | Existing typed query remains missing/ambiguous and never `Available` |

For malformed-input tests, construction should exercise the public semantic
lowering/parser route when that route can express the fact. Direct prepared
record construction is acceptable only in unit tests specifically proving the
admission validator’s fail-closed behavior; it cannot count as positive
capability proof.

## Deterministic acceptance rules

The follow-up is acceptance-ready only when all of these hold:

1. The positive begins as semantic inline assembly and reaches BIR metadata
   without post-prepare mutation or injected regalloc/value-home facts.
2. The explicit operand constraints resolve to two different structured target
   identities before `run_regalloc`.
3. Normalized `PreparedAllocationConstraint` rows preserve value ID, class,
   width, fixed identity, and typed inline-assembly operand provenance.
4. Every normal and eviction candidate-pool pass consumes the normalized row.
5. Assigned registers exactly match the source constraints, independent of
   allocation order, unrelated pressure, or register-pool iteration order.
6. Out-of-SSA publishes a real non-coalesced move from A to B; no test-created
   `PreparedMoveResolution` is used.
7. `current_block_direct_edge_publication_sources` reports the register source
   as `Available` with one selected freshness authority.
8. The full negative matrix remains fail closed with typed statuses.
9. Reordering unrelated BIR definitions or adding an unused allocatable value
   does not change the constrained identities or proof outcome.
10. The proof includes the existing natural coalesced case and confirms it
    still publishes no move; coalescing is not reclassified as publication.

## Forbidden shortcuts

- No preparation option or fixture map from function/value spelling to a
  physical register.
- No post-regalloc assignment rewrite, post-prepare value-home mutation, move
  synchronization, or typed-view inference.
- No block-label, value-name, known-register-pair, or testcase-shape branch.
- No dependence on the allocator choosing A/B incidentally under crafted
  pressure.
- No manual positive `PreparedAllocationConstraint`,
  `PreparedMoveResolution`, publication, or freshness record.
- No target consumer fallback, Route 5 restoration, expectation downgrade, or
  classification of the supported case as unsupported.
- No treating a coalesced input/destination as if an edge move executed.

## Narrow follow-up implementation idea outline

The research supports one follow-up initiative with this boundary:

**Title:** Prepared inline-assembly explicit-register allocation constraints

**Goal:** Preserve supported explicit-register inline-assembly operand meaning
as authenticated prepared requests, normalize it into per-value allocation
constraints, and enforce those constraints in common regalloc.

**In scope:**

- Extend structured inline-assembly operand metadata/lowering only as needed to
  distinguish explicit physical identity from class-only constraints.
- Publish typed prepared requests before `run_regalloc`, keyed by interned
  function/value identity and operand provenance.
- Centralize supported target spelling-to-identity and identity-to-candidate
  span validation for the initially supported target(s).
- Normalize fixed identity into `PreparedAllocationConstraint` and constrain
  every assignment/eviction pool.
- Add the semantic positive and negative matrix above, including the downstream
  direct-edge availability proof.

**Out of scope:**

- General user/test register hints for arbitrary BIR values.
- Preferred-register policy unrelated to a semantic producer.
- Broad inline-assembly syntax expansion, target emission changes, ABI policy,
  publication-query weakening, or joined-branch-specific controls.

**Completion boundary:** the initially supported explicit-register operand
family is end-to-end and deterministic, malformed/conflicting authority fails
closed, and the publication proof is a consequence of semantic constraints.
Other constraint syntaxes or targets require separate follow-ups.

## Stop condition

If the frontend/lowering path cannot preserve a supported explicit-register
operand as structured semantic metadata without a broad inline-assembly
redesign, do not substitute a fixture carrier. Stop the implementation idea and
record that idea 722 lacks a legitimate deterministic register-source producer.
