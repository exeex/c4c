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
