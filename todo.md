# Current Packet

Status: Active
Source Idea Path: ideas/open/731_inline_asm_transport_and_regalloc_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Converge import, core ownership, and Raw publication

## Just Finished

- Plan Step 2 is complete. `core`, `lir_to_bir`, its build-excluded `memory`
  sub-boundary, and the Raw verifier profile now agree on one owned graph and
  epoch/owner/slot/generation identity model, exact one-time source-ID mapping,
  and generic ordinary operand/result edges.
- All four contracts now require one frozen `ModuleDraft` and the sole atomic
  `verify_and_publish_raw(ModuleDraft&&)` transition. Any import, builder,
  verifier, or revision failure publishes no `RawBir` and retains the shared
  structured failure boundary.
- Inline asm now has one lossless Raw carrier: ordinary SSA uses/definitions
  plus original opaque asm/constraint text, ordered clobbers, and side effects.
  Parsing/binding target constraint meaning is explicitly deferred to the
  later revision-bound constraint stage. Current bounded bootstrap behavior is
  separated from the unimplemented general import and full Raw target design.

## Suggested Next

- Execute Plan Step 3 in its declared order: reconcile pipeline, pass, and
  analysis infrastructure beneath the root total order, with transactional
  stage publication, stable-ID preservation, revision binding, and analysis
  invalidation.

## Watchouts

- Do not begin implementation before explicit architecture acceptance.
- Keep the checked-in `ModuleBuilder::publish()` and foundation verifier named
  as temporary bounded bootstrap adapters; they do not satisfy the accepted
  target `ModuleDraft`/full-Raw publication API.
- Do not let later canonical passes parse or normalize inline-asm constraint
  text; root stage `S18` remains the sole constraint interpreter.
- Do not let Step 3 pipeline details supersede the root README's total order.
- Keep idea 731 open when this docs-only runbook is exhausted.

## Proof

- `git diff --check && test "$(rg -l 'RawBir|Raw BIR'
  src/backend/bir/core/README.md src/backend/bir/lir_to_bir/README.md
  src/backend/bir/lir_to_bir/memory/README.md
  src/backend/bir/verify/README.md | wc -l)" -eq 4 && ! rg -n
  'RawBir.*(ABI placement|register allocation|spill/reload|target opcode)|Raw
  BIR.*(ABI placement|register allocation|spill/reload|target opcode)'
  src/backend/bir/core/README.md src/backend/bir/lir_to_bir/README.md
  src/backend/bir/lir_to_bir/memory/README.md
  src/backend/bir/verify/README.md` — exit 0.
- The supervisor explicitly classified this as docs-only proof; no
  `test_after.log` or other regression log was created or modified.
