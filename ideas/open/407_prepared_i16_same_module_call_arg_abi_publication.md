# Prepared I16 Same-Module Call Argument ABI Publication

Status: Open
Type: Follow-up producer repair idea
Parent: `ideas/open/395_rv64_object_route_instruction_fragment_lowering.md`

## Goal

Publish usable prepared scalar integer ABI facts for same-module `i16` call
arguments so RV64 object emission can consume a target-ready call-argument
destination instead of reconstructing producer ABI policy.

## Why This Exists

While executing active idea 395, `src/divmod-1.c` was classified as a
same-module small-integer call-argument publication gap. The prepared facts for
the failing `i16` callsites show:

- `wrapper_kind=same_module`
- `arg index=0 value_bank=none`
- `source_encoding=frame_slot`
- `dest_bank=none`
- move reason `call_arg_stack_to_stack`

A minimized same-module `i8` register-argument case compiles, but a minimized
same-module `i16` case fails with `unsupported_instruction_fragment`. The callee
`i16` formal has a register home (`a0`), so the producer should publish a
target-consumable scalar argument destination for the caller-side same-module
argument rather than leaving object emission to infer it from callee type and
argument position.

This is related to closed
`ideas/closed/403_prepared_i16_formal_abi_publication.md`, but it covers the
caller-side same-module call-argument publication path instead of incoming
formal publication.

## In Scope

- Inspect prepared same-module call-argument planning for scalar integer
  widths, especially the path that publishes `i8` register arguments and
  currently leaves `i16` frame-slot arguments with no value or destination
  bank.
- Extend producer-side prepared ABI publication for same-module `i16` scalar
  arguments so the call plan exposes a target-consumable GPR argument
  destination when the ABI requires one.
- Preserve prepared move-bundle ownership and make any required caller-side
  frame-slot-to-register publication explicit before RV64 object emission.
- Prove `src/divmod-1.c` and the minimized `i16` same-module call artifact no
  longer fail because of the no-bank `call_arg_stack_to_stack` shape.
- Keep the closed 403 incoming-formal repair intact and avoid reopening it
  unless fresh evidence proves the formal path regressed.

## Out Of Scope

- Teaching `src/backend/riscv/rv64/object_emission.cpp` to infer scalar
  call-argument registers from argument index, type width, or callee formal
  homes.
- Broad same-module aggregate, byval, sret, variadic, or stack-passed argument
  redesign.
- Treating unrelated instruction-fragment lowering, move-bundle target, or
  parameter-home failures as part of this producer repair.
- Expectation rewrites, unsupported marker changes, allowlist filtering, or
  filename-specific handling for `src/divmod-1.c`.

## Acceptance

- Same-module `i16` scalar call arguments publish usable prepared ABI/call-plan
  facts before RV64 object emission, including a target-consumable destination
  for ABI-register arguments.
- `src/divmod-1.c` no longer fails with
  `unsupported_instruction_fragment` because an `i16` call argument remains
  `value_bank=none`, `source_encoding=frame_slot`, and `dest_bank=none`.
- A minimized same-module `i16` call artifact that previously failed now
  reaches RV64 object codegen through the repaired producer path, while the
  existing minimized `i8` call case remains supported.
- The RV64 object emitter remains a consumer of prepared facts and does not
  gain a duplicate scalar call-argument ABI classifier.
- Narrow build/proof plus supervisor-selected backend proof are recorded in
  canonical `todo.md` / `test_after.log` by the executor.

## Reviewer Reject Signals

- Reject fixes that special-case `src/divmod-1.c`, `div2`, `div4`, `mod2`,
  `mod4`, or the minimized artifact filenames instead of repairing reusable
  same-module `i16` call-argument ABI publication.
- Reject RV64 `object_emission.cpp` changes that infer the missing argument
  register or destination bank from argument order, type width, or callee
  formal homes; that reconstructs producer ABI policy in the consumer.
- Reject expectation downgrades, unsupported marker changes, or allowlist
  filtering claimed as progress on this idea.
- Reject helper renames, diagnostic wording changes, or classification-only
  updates that leave the same `value_bank=none` / `dest_bank=none`
  `call_arg_stack_to_stack` shape for same-module `i16` scalar arguments.
- Reject broad ABI rewrites that alter unrelated scalar widths, aggregates,
  variadic calls, or stack-passed arguments without local proof and explicit
  plan scope.
- Reject a green `src/divmod-1.c` result if prepared dumps still show the old
  no-bank same-module `i16` call-argument publication hidden behind a new
  abstraction name.
