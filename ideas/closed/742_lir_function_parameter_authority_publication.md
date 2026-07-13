# LIR Function Parameter Authority Publication

Status: Closed (complete)
Type: structured LIR producer-authority decomposition
Blocked Consumer: ideas/open/734_lir_to_new_bir_container_completeness.md

## Goal

Publish every function definition's existing HIR logical parameter facts into
`LirFunction.params` with the same structured ownership already used for
declarations, then prove the exact relationship among logical parameters,
structured ABI signature parameters, and typed signature mirrors without
recovering semantics from names or rendered signature text.

## Why This Exists

Idea 734 reached its first parameter-signature receiver packet and required
exact structured parity among `LirFunction.params`, `signature_params`, and
`signature_param_type_refs` for plain fixed scalar functions. Production
declarations satisfy the logical side by calling
`populate_lir_function_params`, but the definition shell in
`src/codegen/lir/hir_to_lir/hir_to_lir.cpp` populates only the structured ABI
signature tracks. A focused unused four-scalar definition therefore publishes
`params=0`, `signature_params=4`, and `signature_param_type_refs=4`.

The receiver must not infer the missing logical facts from `%p.*` display
names, signature rendering, position alone, or the ABI-oriented signature
track. This separate initiative owns the producer publication seam and a
bounded authority classification before handing exact receiving facts back to
idea 734.

## Authority Contract

- HIR `Function::params` is the structured logical parameter authority.
- `LirFunction.params` preserves logical parameter order, count, and owned
  `TypeSpec`; its string name is presentation only.
- `LirFunction.signature_params` and
  `LirFunction.signature_param_type_refs` preserve the structured ABI
  signature and its typed mirror. They are not substitutes for missing logical
  parameters.
- Plain fixed one-to-one scalar parameters must agree across all three tracks
  in order, count, and type.
- Zero-parameter and explicit-void lists must retain their distinct structured
  states and declaration/definition parity.
- ABI expansion may intentionally make logical and signature counts or types
  differ. Such rows require an explicit disposition, not forced one-to-one
  parity or receiver inference.

## In Scope

- call the existing `populate_lir_function_params` helper for function
  definitions from the same structured HIR facts used by declarations
- prove declaration/definition logical publication parity for empty,
  explicit-void, and unused fixed scalar parameter lists
- prove plain `i32`, `i64`, `float`, and `double` parameter order/count/type
  agreement across logical params, structured signature params, and typed
  signature mirrors
- use misleading or duplicate-looking parameter spellings to prove names and
  rendered signature text are not semantic authority
- add reachable structured producer and LIR-verifier coverage for missing,
  reordered, or type-conflicting authoritative state where the relationship is
  one-to-one
- classify pointer, narrow-integer, byval, HFA/vector/aggregate or otherwise
  ABI-expanded, and variadic shapes by logical carrier, ABI carrier,
  declaration/definition state, and safe receiver disposition
- record a concise handoff naming the exact idea-734 parameter rows unblocked
  and those that remain fail-closed

## Focused Probes

1. Empty and explicit-void declaration/definition pairs: preserve the
   distinction between an empty logical list and the explicit-void logical
   sentinel while the ABI signature uses its native void-list flag.
2. Unused fixed scalar declaration/definition pairs: prove exact ordered
   structured publication for `int`, `long long`, `float`, and `double`
   independently of body parameter uses.
3. Misleading names: prove repeated, sanitized, or type-suggesting display
   spellings never determine identity or type.
4. Classification neighbors: pointer, `char`/`short` extension-sensitive,
   byval aggregate, HFA/vector/other ABI expansion, and variadic fixed-prefix
   forms retain truthful structured dispositions without broad receipt work.

## Progress Contract

The first implementation seam is the definition-side call to the existing
logical-parameter producer plus direct structural tests. Later work may harden
verification and classification, but no receiver edit is capability progress
for this initiative. Completion requires a checked handoff that distinguishes
one-to-one parity from intentional ABI expansion.

## Out Of Scope

- any new-BIR container, builder, verifier, importer, or parameter receipt
  change; those remain owned by idea 734
- relaxing idea 734's exact-parity checks or accepting missing logical params
- reconstructing parameter identity or type from names, `signature_text`,
  `LirTypeRef::str()`, LLVM rendering, or body operand text
- parameter-body use identity, local/stack-object identity, CFG receipt, calls,
  return receipt, canonical BIR, ABI placement, target preparation, allocation,
  MIR, emission, or assembler work
- redesigning `LirFunction`, replacing the logical and ABI parameter tracks,
  or forcing one logical parameter to equal multiple ABI-expanded parameters
- implementing broad pointer, narrow-integer, byval, aggregate, vector, HFA,
  va-list, function-pointer, or variadic receiver support
- expectation downgrades, supported-to-unsupported changes, named-case
  dispatch, or testcase-shaped producer branches

## Acceptance Criteria

- Production definitions and declarations both populate `LirFunction.params`
  from HIR structured parameter facts through the ordinary shared helper.
- Empty and explicit-void parameter lists preserve their distinct states and
  have matching declaration/definition publication.
- An unused fixed `int`/`long long`/`float`/`double` definition and declaration
  preserve exact logical order/count/type and agree with the one-to-one
  structured signature parameters and typed mirrors.
- Misleading display names and signature text cannot change the structured
  result; malformed missing, reordered, or conflicting one-to-one authority
  rejects through reachable verification.
- Pointer, narrow-integer, byval, ABI-expanded/HFA/vector/aggregate, and
  variadic shapes have a checked truthful classification that does not claim
  receiver support or erase intentional logical-versus-ABI distinctions.
- A handoff names the exact idea-734 rows now safe to receive and leaves every
  unproven row fail-closed.
- A fresh build, focused frontend/LIR proof, relevant backend fail-closed
  boundary proof, and supervisor-selected broader regression checkpoint pass.

## Reviewer Reject Signals

- Reject any receiver-side relaxation, inferred fallback, or backend mapping
  that accepts `params=0` for a nonempty plain definition instead of fixing the
  producer publication seam.
- Reject parsing or comparing `%p.*` names, `signature_text`, rendered LLVM
  types, `LirTypeRef::str()`, body operand spelling, or testcase filenames to
  invent logical identity, count, order, or type.
- Reject a named-case branch or helper call restricted to the focused unused
  scalar testcase instead of the ordinary definition shell shared by all
  definitions.
- Reject declaration regressions, duplicate population, reordered parameters,
  borrowed HIR type state, or a definition path that still omits
  `LirFunction.params` behind a renamed helper or abstraction.
- Reject forcing one-to-one parity onto byval, HFA, vector, aggregate, or other
  ABI-expanded signatures; their logical and ABI tracks must remain distinct
  and explicitly classified.
- Reject expectation downgrades, supported-to-unsupported changes, weaker
  malformed-input contracts, allowlists, or test edits that make the old
  `params=0` failure disappear without publishing structured authority.
- Reject helper renames, classification-only text, or mirror-only test changes
  claimed as producer capability while production definitions remain empty.
- Reject broad LIR schema redesign, parameter-body identity, CFG/local-object
  work, downstream ABI placement, canonicalization, allocation, MIR, or
  emission under this bounded initiative.
- Reject completion proof limited to one target testcase; require nearby
  zero/void, declaration/definition, misleading-name, scalar-order, and
  classified complex-shape coverage.

## Completion Evidence

- Commit `6b102a522` publishes definition logical parameters through the same
  structured `populate_lir_function_params` helper used by declarations.
- Commit `96c7892dd` adds reachable exact-parity verification only for proven
  default-shape nonvariadic plain fixed scalars, including malformed count,
  order, type, shape, and typed-mirror rejection.
- Commit `c2024bf49` proves zero/void distinction, declaration/definition
  parity, misleading-display independence, and truthful pointer, narrow,
  aggregate/byval, HFA expansion, and variadic classification.
- Commit `d80d381a0` records the checked authority matrix and exact bounded
  receiver handoff in
  `docs/lir_function_parameter_authority/handoff_to_734.md`.
- Supervisor acceptance used a fresh default build; focused
  `frontend_lir_function_signature_type_ref` and
  `backend_lir_to_bir_interface` proof passed 2/2; production `--dump-bir`
  probes remained fail-closed at `UnsupportedFunctionParameters`; and the full
  suite passed 3033/3033 with monotonic delta 0/0 and no new over-30-second
  tests.
- Closure proves producer publication, exact plain-shape verification, and
  classification only. It does not claim new-BIR parameter receipt or body
  parameter identity. Idea 734 resumes only for zero/void shape plus fixed
  default-shape nonvariadic plain scalar declaration/definition signatures.
