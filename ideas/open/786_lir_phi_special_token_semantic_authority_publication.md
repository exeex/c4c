# LIR PHI Special-Token Semantic Authority Publication

Status: Open
Type: bounded LIR operand-schema and PHI producer/verifier authority repair
Predecessor: `ideas/open/734_lir_to_new_bir_container_completeness.md`

## Goal

Publish native, typed semantic authority for each relevant
`LirOperandKind::SpecialToken` PHI incoming operand so a downstream receiver
can preserve that fact without deriving it from display spelling or
classification.

## Why This Exists

Closed 751 supplies typed `LirOperand` and predecessor-block authority for PHI
incoming rows, but a SpecialToken `LirOperand` still has `std::monostate`
authority. Its `SpecialToken` kind is selected only from its display spelling
(`null`, `undef`, `poison`, `zeroinitializer`, `true`, or `false`), while
`LirOperandAuthority` currently carries only `LirValueId`, `LinkNameId`, or
`LirIntegerImmediate`. The active 734 Raw-BIR PHI receiver may not infer that
missing semantic fact from spelling, printer output, LLVM text, or test shape.

## In Scope

- Define one native, non-text semantic authority representation for the
  existing SpecialToken vocabulary relevant to PHI incoming operands.
- Extend `LirOperandAuthority` and the `LirOperand` construction/access API so
  a SpecialToken carries that authority while preserving display text only as a
  mirror.
- Update the relevant PHI producers to publish this authority and update LIR
  verification to require coherent SpecialToken kind/authority and reject
  absent, mismatched, or invalid authority before downstream use.
- Add focused positive and malformed-authority proof for PHI-producing routes
  that use these special tokens, including a misleading-display case that
  demonstrates native authority, not spelling, governs identity.
- Produce a compact handoff naming the typed authority, covered PHI producer
  routes, verifier rejects, proof, and exact 734 return point.

## Out Of Scope

- Raw-BIR containers, Raw-BIR verifier or importer changes, Step 7.25 receiver
  work, backend lowering, MIR, emission, or target interpretation.
- Textual recovery from labels, token spelling, printer output, LLVM text,
  instruction order, or testcase names.
- General non-PHI operand migration, ordinary SSA/direct-constant/immediate
  redesign, value/CFG identity changes, and changes to 751's predecessor-edge
  contract.
- Adding unsupported token spellings, canonicalization, or broad operand-model
  rewrites beyond the existing classified SpecialToken vocabulary.

## Acceptance Criteria

- Every in-scope PHI SpecialToken operand carries a native typed semantic
  authority that distinguishes the existing classified token forms without
  inspecting its display text.
- Relevant producers publish the authority and LIR verification rejects absent,
  kind-mismatched, invalid, and display-misleading authority.
- The focused proof covers both positive PHI production and malformed
  authority; it proves the handoff is producer/verifier authority only, not
  Raw-BIR receipt.
- The handoff lets 734 resume unchanged at Step 7.25 with the exact native
  authority and no presentation recovery.

## Reviewer Reject Signals

- Reject any Raw-BIR receiver, importer, backend-lowering, MIR, emission, or
  target-semantics diff claimed as this authority publication.
- Reject a representation that preserves only `SpecialToken` classification or
  display spelling, parses text at a consumer, or uses printer/LLVM/testcase
  text as semantic identity.
- Reject producer coverage limited to a named testcase or a single token while
  claiming the existing classified SpecialToken PHI vocabulary is published.
- Reject expectation downgrades, unsupported fallbacks, helper-only renames,
  or verifier omissions claimed as authority progress.
- Reject a generic operand-model rewrite or changes to non-PHI families that
  are not required to publish and validate this bounded authority.

## Initial Execution Contract

### Step 1 - Publish and verify PHI SpecialToken authority

Goal: add the smallest native SpecialToken authority carrier and make relevant
PHI producers/verifier publish and enforce it.

Primary targets:

- `src/codegen/lir/operands.hpp`
- relevant PHI-producing LIR construction sites and `src/codegen/lir/verify.cpp`
- focused LIR producer/verifier coverage

Actions:

- enumerate the existing classified SpecialToken forms used by relevant PHI
  rows; encode their semantic identity in a native typed authority alternative,
  with display text retained only as a non-authoritative mirror
- make only those PHI-producing paths publish the new authority and reject
  missing, mismatched, invalid, or display-misleading authority in verification
- add nearby positive and malformed tests, then record the exact authority and
  coverage in the handoff to 734

Completion check:

- a fresh build and focused producer/verifier proof establish native
  SpecialToken authority for the selected PHI rows; no Raw-BIR/importer change
  is present.
