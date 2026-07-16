# HIR Function-Signature Direct Aggregate-Ref Carrier

Status: Open
Type: bounded upstream HIR semantic-construction carrier prerequisite
Blocked Parent: `ideas/open/848_hir_aggregate_occurrence_canonical_ref_population.md`, Step 2b

## Goal

Carry an already definition-backed HIR aggregate reference or its authoritative
HIR definition through the function-signature semantic/construction API to the
HIR lowering sites for function returns and parameters, so the parent can pass
that direct carrier to `qtype_from` without recovering identity from a type.

## Why This Exists

848 Step 2a established a legal direct input at `qtype_from`, but its
function-signature call sites receive only normalized `TypeSpec`. The existing
callable-definition resolver derives candidates through `record_def`,
structured owner metadata, and legacy tag routes. Those recovery paths are
explicitly forbidden for canonical `HirAggregateRef` authority. The missing
work is a distinct upstream semantic-construction carrier, not signature
occurrence population and not LIR lowering.

## In Scope

- Trace only the function return and parameter semantic/construction path from
  an already registered, module-owned HIR aggregate definition to the HIR
  lowering call sites.
- Add the smallest explicit HIR semantic/construction carrier/API that conveys
  an existing complete, module-owned `HirAggregateRef`, or the HIR definition
  that authoritatively owns it, to those call sites.
- Define fail-closed carrier behavior for absent, incomplete, invalid, or
  foreign/module-mismatched aggregate facts.
- Add focused same-feature proof that both return and parameter paths receive
  the direct definition-backed carrier, including malformed-boundary coverage.
- Return only the accepted carrier/API and evidence to 848 Step 2b; 848 owns
  passing it into `qtype_from` and observing occurrence population.

## Out Of Scope

- Any parser, `TypeSpec`, `record_def`, structured-owner, legacy owner-key,
  tag, rendered-text, parser-pointer, `Node*` map, or reconstructed lookup for
  canonical aggregate identity.
- Changing `qtype_from` occurrence population, `QualType::aggregate_ref`
  attachment, or 848 Step 2b's return/parameter lowering edits.
- LIR construction, lowering, aggregate-store consumers, verifier/printer
  work, or changes to 838, 836, or 831.
- Other signature positions, other nominal families, broad type-system
  refactors, or expectation/test-harness weakening.

## Acceptance Criteria

- Function-signature semantic/construction state exposes one explicit direct
  carrier from an already registered module-owned HIR aggregate definition to
  both return and parameter HIR lowering call sites.
- The carrier never derives identity from parser or normalized-type metadata;
  missing, incomplete, invalid, and foreign values fail closed at its boundary.
- Focused positive coverage exercises return and parameter carrier delivery,
  while malformed-boundary coverage proves no forbidden recovery occurs.
- The completion record names 848 Step 2b as the exact return and states that
  848 alone will pass the carrier into `qtype_from`; it makes no LIR claim.

## Reviewer Reject Signals

- Reject parser/`TypeSpec`/`record_def`, structured-owner, owner-key, tag,
  text, parser-pointer, `Node*` map, or reconstructed lookup presented as a
  direct semantic carrier.
- Reject populating `QualType::aggregate_ref`, changing `qtype_from`, or
  altering function-signature lowering under this prerequisite; those actions
  belong to 848 Step 2b after return.
- Reject any LIR migration, aggregate-store consumer change, verifier/printer
  change, or adjacent nominal-family refactor as carrier progress.
- Reject named-test-only routing, expectation downgrades, malformed-case
  acceptance, or a renamed abstraction that still resolves identity from
  forbidden metadata.

## Resumption Record: 850 no-change conclusion

Status: resumed after the intentional no-change conclusion of
`ideas/closed/850_hir_signature_aggregate_ref_producer_order.md`.

- Last accepted progress: Step 1 — **Locate the signature semantic carrier
  seam** — is accepted at `109ea13f4` (`plan: locate direct signature aggregate
  ref carrier seam`). It identified the `lower_function` carrier/API boundary
  and the direct HIR-definition validation contract.
- Interrupted step: Step 2 — **Add the bounded direct carrier/API**. No Step 2
  implementation is accepted, and there is no accepted 849 code or proof.
- Rejected fact: every production `lower_function` caller supplies the default
  null carrier. A carrier that merely validates then holds/discards an input
  cannot deliver a definition-backed fact to either lowering site and is not
  capability progress.
- 850 conclusion: tracing `src/frontend/hir/hir_build.cpp` established that
  every production caller supplies only `Node`/template inputs to
  `Lowerer::lower_function`; no caller has a semantic aggregate fact to pass.
  The only module-issued direct refs are on registered `HirStructDef` objects.
  Without the carrier/API delivery owned here, a fact cannot reach the seam
  without forbidden `Node`/`TypeSpec`/record/tag/owner lookup, maps, or
  reconstruction. Therefore 850's purported independent pre-carrier producer
  route is intentionally concluded with no code change, not accepted as a
  capability.
- Exact return point: resume at Step 2 — **Add the bounded direct carrier/API**.
  Define the carrier and make the production `hir_build.cpp` callers pass an
  already-issued definition/ref when their construction context has one (else
  no fact); then forward it through `lower_function` to both signature
  positions. Do not redo Step 1 and do not add parser/`TypeSpec`/owner/tag/text
  lookup, a `Node*` map, `qtype_from` attachment, or LIR work.
- Parent record: 848 remains parked. Its accepted Step 2a implementation and
  focused `frontend_hir_tests` proof are `359a9b94b`; 849 has no accepted code
  or proof to add to that record.
