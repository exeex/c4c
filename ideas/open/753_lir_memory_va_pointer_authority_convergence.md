# LIR Memory And VA Pointer Authority Convergence

Status: Open
Type: bounded LIR memory/va pointer authority repair
Predecessor: `ideas/open/752_lir_local_object_pointer_authority_convergence.md`

## Goal

Move memory intrinsic and va-list operations away from text-only pointer
operands by consuming structured pointer/object/lifetime authority from the
local/object model.

## Why This Exists

`LirMemcpyOp`, `LirMemsetOp`, `LirVaStartOp`, `LirVaEndOp`, `LirVaCopyOp`, and
`LirVaArgOp` still mostly carry monostate/text operands. Selected memcpy work
handles one row, but the broader family will keep reintroducing string-based
semantic gaps unless the common memory/va pointer boundary is closed.

## In Scope

- Generalize from the selected memcpy row only after idea 752 supplies the
  local/object pointer substrate.
- Publish structured pointer/object/lifetime authority for representative
  memcpy, memset, va_start, va_end, va_copy, and va_arg routes.
- Preserve typed size/value operands where applicable and reject malformed
  size, pointer, owner, and lifetime authority.
- Keep unconverted memory/va rows fail-closed or explicitly compatibility-only.
- Add focused positive and negative coverage across memory and va-list
  producers, including at least one vaarg route that emits memcpy-like moves.

## Out Of Scope

- Raw-BIR receiver work, target lowering, MIR, emission, alias analysis, or
  full memory model semantics.
- CFG/PHI work, local/object substrate definition, aggregate/vector carrier
  publication, or opaque inline-asm text.
- Treating builtin names, operand spelling, rendered LLVM, or testcase shape as
  semantic authority.

## Acceptance Criteria

- Representative memory and va-list operations consume structured pointer and
  lifetime authority instead of parsing text.
- The verifier rejects invalid, foreign, type-mismatched, size-mismatched, or
  dead pointer/object authority.
- The selected memcpy route remains a special proven row, not the only memory
  operation with structured authority.
- Full baseline acceptance requires 100% passing tests. If a baseline run is
  below 100%, reject closure and trace `log/*` by time/commit to identify the
  first bad commit before continuing.

## Blocked Resumption Record

- **Last accepted progress:** 753 Step 2 accepted `va_start`/`va_end` in
  `8f6f4f9c9`, positive-size aggregate `memset` baseline repair in
  `0912c93a9`, `va_copy` in `17ba5b129`, and AMD64 scalar/pointer `va_arg` in
  `52f143765`. The latter's backend guard passed 5/5 and its accepted full
  baseline passed 3037/3037.
- **Interrupted step:** Step 2, *Publish and verify native
  pointer/object/lifetime authority*.
- **Blocker:** `ideas/open/799_lir_amd64_vaarg_overflow_aggregate_carrier_authority.md`.
  The source-required aggregate `va_arg` memcpy-like route in
  `emit_amd64_va_arg_from_overflow`
  (`src/codegen/lir/hir_to_lir/call/vaarg_amd64.cpp`) emits
  `LirMemcpyOp{tmp_addr, stack_ptr, size}`, where `stack_ptr` is an
  overflow-area-derived pointer rather than a current-function local-object
  pointer. Its structured derived carrier authority is aggregate/vector carrier
  work expressly outside 753's scope.
- **Exact return point:** after the blocker supplies a checked structured
  derived overflow-area pointer/object/lifetime and typed-size carrier for the
  AMD64 aggregate `va_arg` memcpy row, resume 753 Step 2 and select only that
  matching producer packet. Step 3 remains the later proof/handoff step.
- **Remaining work:** publish and verify only the matching 753 producer row
  against the blocker contract, then complete 753 Step 3 proof and its one
  receiver handoff. Do not bridge this gap with text recovery or expand 753 to
  aggregate/vector carrier work.
