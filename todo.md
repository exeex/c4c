# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.24
Current Step Title: Publish implicit integer call-argument cast authority

## Just Finished

- Stopped Plan Step 7.23 without code changes and classified scalar inline-asm
  inputs as an exact separately owned blocker.
- The current binding can preserve a CC-LOAD-1 ID plus native type, Input role,
  and argument index, but removing the sole `ordinary_inputs` entry produces
  the same native shape as legitimate zero-input void inline assembly. The
  intended input count exists only in authority-free opaque constraint/args
  text.
- Truthful missing/count rejection therefore requires a new native semantic
  input-shape/count carrier. That belongs to a separate future inline-assembly
  schema initiative; a constraint/argument-text bridge is forbidden.
- Step 7 remains active: the matrix still lists implicit scalar coercion casts
  as unclaimed, and one fixed direct-call argument widening is carrier-ready on
  accepted load, scalar cast, structured argument, and signature authority.
- Builtin parity and scalar multi-output inline assembly retain their exact
  separately owned blockers and are not reopened here.

## Suggested Next

- Executor: complete Plan Step 7.24 for only one fixed nonvariadic direct void
  call whose selected-global signed i32 argument is implicitly widened to i64.
  Preserve the exact CC-LOAD-1 result into an authoritative SExt, allocate the
  cast through `fresh_value`, and carry that exact cast result ID into the
  corresponding structured call argument.

## Watchouts

- Own only PC's fixed scalar argument-preparation route for exact signed
  i32-to-i64 SExt. Require native SExt kind, exact i32/i64 endpoint refs, one
  exact fixed i64 signature parameter, and argument index/count agreement.
- Reuse the accepted `coerce_operand` and structured `OwnedLirTypedCallArg`
  carriers. Do not reopen or weaken CC-LOAD-1, Step 7.1 explicit casts, or
  Steps 4-5 direct void argument contracts.
- Preserve the exact load-to-SExt-to-call-argument ID chain. Rendered argument,
  cast, call, callee, and temporary spelling may observe native facts but must
  never create, repair, select, or validate authority.
- Reachable verification must reject invalid/duplicate cast results; missing,
  wrong-alternative, unknown, or cross-function source/use IDs; wrong cast
  kind/endpoints; and conflicting callee, signature, count/index, parameter
  type, or structured argument type. Misleading displays with unchanged native
  facts must pass.
- Exclude unsigned extension, narrowing, same-representation no-ops, return or
  assignment coercions, floating/pointer/bitcast/vector/aggregate conversions,
  immediate and body-parameter sources, indirect/variadic/ABI calls, other
  argument positions, inline assembly, BIR receipt, parity, and all idea-741
  work beyond preserving CC-LOAD-1 unchanged.
- This packet is carrier-ready only on the accepted selected-global load,
  scalar integer SExt, fixed structured argument, direct callee/signature, and
  generic ownership seams. If the selected route needs a new coercion-purpose
  carrier or any name/text-derived bridge, stop and return that exact blocker
  rather than widening Step 7.24.

## Proof

- Fresh `cmake --build --preset default`.
- Focused `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure`
  with a selected-global-load-to-implicit-SExt-to-fixed-argument chain,
  misleading-display positives, and malformed result/source/use, cast fact,
  signature, count/index, and argument-type cases.
- Preserve CC-LOAD-1, Step 7.1, Steps 4-5, neighboring call coercions, and
  accepted scalar identity coverage unchanged; the supervisor owns matched
  regression logs and any broader/full proof.
- Run `git diff --check` before handoff.
