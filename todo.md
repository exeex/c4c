# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.26
Current Step Title: Publish scalar read-write inline-asm binding authority

## Just Finished

- Accepted Plan Step 7.25 as width-generic production proof for PS's single
  non-explicit-register output-only scalar i64 `LirInlineAsmOp` binding and
  later type-matched Store use.
- No production or schema specialization was needed: the accepted Step-7.21
  output mechanism allocated the i64 result through `fresh_value`, retained
  exact native Output/type/index facts, and preserved the exact Store edge.
- Focused and matched full proof passed, and commit `8f6c25ff0` records the
  coherent Step-7.25 slice.
- Step 7 is not complete: a single scalar read/write binding remains an
  unclaimed production disposition. Its paired native `ReadWrite` roles and
  shared constraint index make the old-value/new-value shape carrier-ready
  without opaque-text or fixture inference.
- Parity, inline-asm multi-output/input-only, and producer-specific implicit
  coercion retain their exact separately owned blockers and are not reopened.

## Suggested Next

- Executor: complete Plan Step 7.26 for only one PS single-output scalar i32
  non-explicit-register read/write `LirInlineAsmOp` binding on a selected
  global. Preserve the exact CC-LOAD-1 ID as the old-value semantic input,
  allocate a distinct new-value semantic result through `fresh_value`, and
  preserve that exact result ID into the later type-matched Store.

## Watchouts

- Require exactly one native i32 `ReadWrite` input and one native i32
  `ReadWrite` result at constraint index zero. Their IDs must be valid and
  distinct; their role, type, index, and paired collection positions/counts
  must agree.
- The input use must resolve to the exact current-function selected-global load
  result. The result is a new function-owned definition and the later Store
  must consume that exact new ID, never the old input ID.
- Preserve the selected-global load contract unchanged. Claim only the Store's
  scalar value-use edge; do not expand local/object/pointer or general Store
  authority.
- Compatibility `result`, rendered operands/arguments, original assembly and
  constraint text, clobbers, and every other mirror remain presentation/
  opaque. They may observe native facts but never create, repair, select, pair,
  or validate authority.
- Reachable verification must reject missing or wrong-alternative input/result
  authority; invalid, duplicate, equal, unknown, or cross-function IDs; role,
  type, index, pair-count/position conflicts; a result used as the old input;
  an old input used by the Store; and Store type conflicts. Misleading displays
  with unchanged native facts must pass.
- Exclude output-only Steps 7.21/7.25, input-only bindings, multi-output,
  multiple-input, memory/address/immediate/clobber, explicit-register,
  floating/vector/aggregate bindings, `insn_r`, opaque-text interpretation,
  compatibility-result authority, stack/local/object/body parameters, CFG,
  BIR receipt, parity, and all idea-741 work beyond preserving CC-LOAD-1.
- This packet is carrier-ready only because the existing paired native
  `ReadWrite` roles and shared index identify both sides. If truthful pairing
  or old/new distinction requires constraint-text inference or a new semantic
  carrier, stop and return that exact blocker rather than widening Step 7.26.

## Proof

- Fresh `cmake --build --preset default`.
- Focused `ctest --test-dir build -R '^frontend_hir_tests$' --output-on-failure`
  with a selected-global-load-to-read/write-input, distinct-result-to-Store
  chain, misleading-display positives, and malformed pairing, identity,
  role/type/index/count/position, old/new edge, and Store-use cases.
- Preserve Steps 7.21/7.25 output-only behavior, CC-LOAD-1, neighboring
  inline-asm metadata/diagnostics, and accepted generic identity coverage
  unchanged; the supervisor owns matched regression logs and broader proof.
- Run `git diff --check` before handoff.
