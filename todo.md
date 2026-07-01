Status: Active
Source Idea Path: ideas/open/512_stack_passed_parameter_home_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Trace Producer ABI Home Publication

# Current Packet

## Just Finished

Step 2 of `plan.md` traced the producer ABI/prealloc path for
stack-passed parameter homes without changing implementation code.

Producer fact path:

- Frontend/lowering already carries formal/call ABI metadata in
  `bir::Param::abi` and `bir::CallInst::arg_abi`.
- `src/backend/prealloc/regalloc/call_return_abi.cpp` classifies each call
  argument with `call_arg_storage_kind`, computes ABI register indexes with
  `call_arg_abi_register_index`, and should be the shared source of stack
  destination offsets through `call_arg_destination_stack_offset_bytes`.
- `src/backend/prealloc/regalloc.cpp` publishes call-site ABI bindings in
  `append_prepared_call_abi_bindings`; stack-slot bindings already keep
  `destination_storage_kind=StackSlot` and use
  `destination_stack_offset_bytes` when the helper returns an offset.
- `src/backend/prealloc/regalloc/call_moves.cpp` publishes matching
  `PreparedMoveResolution` records for call-argument moves; it also uses
  `call_arg_destination_stack_offset_bytes`, so RV64 currently gets
  offsetless stack destinations at both the ABI-binding and move-resolution
  surfaces.
- `src/backend/prealloc/regalloc/value_homes.cpp` publishes callee fixed
  formal register homes in `classify_prepared_value_home` by matching
  `PreparedRegallocValue` parameter names to `bir::Function::params` and
  deriving the ABI register name. Ordinary RV64 fixed formals passed on the
  incoming stack have no equivalent branch, so they fall through to later
  allocation state or `PreparedValueHomeKind::None`.
- `src/backend/prealloc/formal_publications.cpp` already validates formal
  publication plans for `IncomingStackToHome`; it fails closed with
  `MissingStackOffset`, `MissingValueHome`, `MissingAbiInfo`,
  `UnsupportedHomeKind`, or `UnsupportedFormalSource` when required producer
  facts are absent.

Where facts are preserved or lost:

- Register homes are already preserved at both caller and callee surfaces:
  caller bindings/moves get register names, occupied register names, register
  placements, and contiguous width; callee formals get `Register` homes and
  RV64 target-register identity where supported.
- Caller stack homes lose offset authority because
  `call_arg_destination_stack_offset_bytes` returns `nullopt` for RV64 before
  walking the ABI arguments. `append_prepared_call_abi_bindings` and
  `append_call_arg_move_resolution` therefore publish stack-slot destination
  kind and ABI index but no `destination_stack_offset_bytes`.
- Callee stack homes lose storage kind/slot facts because the fixed-formal
  branch in `classify_prepared_value_home` only special-cases variadic homes,
  F128 stack locals, and register-passed fixed formals. Ordinary fixed
  `passed_on_stack` scalar formals are not mapped to a prepared incoming
  stack-slot home with offset, size, and alignment.
- Width/alignment authority exists in `bir::CallArgAbiInfo` and the existing
  stack argument sizing helpers, but it is not yet connected to RV64 caller
  stack offsets or callee fixed-formal stack homes.

## Suggested Next

Execute Step 3 by adding producer-side RV64 stack-argument/formal publication:

- Extend the shared ABI helper boundary in
  `src/backend/prealloc/regalloc/call_return_abi.cpp` so ordinary RV64
  stack-passed call arguments that already require stack destinations receive
  explicit destination stack offsets. Reuse the existing stack-argument size
  and alignment helpers rather than teaching RV64 object emission an ABI
  formula.
- Add a narrow fixed-formal stack-home helper in
  `src/backend/prealloc/regalloc/value_homes.cpp` that maps ordinary
  non-varargs, non-byval, non-sret RV64 formals with `param.abi->passed_on_stack`
  to `PreparedValueHomeKind::StackSlot` only when the producer can publish
  coherent offset, size, alignment, and frame-slot/home identity.
- Keep `append_prepared_call_abi_bindings`,
  `append_call_arg_move_resolution`, and `plan_prepared_formal_publication` as
  consumers of those producer facts. RV64 should continue consuming prepared
  records and should not infer offsets from argument index, parameter name, or
  testcase shape.
- Add focused prepared-contract tests around caller stack ABI bindings and
  callee fixed-formal stack homes before any RV64 object-route widening.

## Watchouts

- Keep the route limited to producer ABI/prealloc publication of explicit
  stack-passed parameter homes.
- Do not infer stack argument homes in RV64 from argument indexes, ABI
  formulas, source call shape, parameter names, or named gcc_torture rows.
- Preserve existing fail-closed behavior for missing stack offsets, ambiguous
  or duplicate homes, missing value homes, missing ABI metadata, unsupported
  varargs, F128 stack-passed local-home special cases, byval/sret/aggregate
  ABI paths, dynamic stack cases, and unrelated call/result shapes.
- Keep varargs, F128, aggregate ABI, dynamic stack work, broad RV64 call
  lowering, and unrelated ABI repairs out of this plan unless the supervisor
  splits a new source idea.
- `20001017-1.c` currently trips `fpr:fs1` save-slot rejection before the old
  `unsupported_param_home` check, so Step 2 should use prepared dumps and
  focused synthetic probes to prove the producer facts instead of treating the
  current object diagnostic as evidence that the param-home gap disappeared.
- RV64 already has consumer validation for prepared formal stack-slot homes;
  the next slice should not add RV64 inference from raw ABI indexes or source
  shape.

## Proof

- Trace-only packet; no build was required and no root-level proof log was
  rewritten.
- Inspected the active `plan.md`, `todo.md`, Step 1 evidence notes, and the
  producer/consumer files named by the packet:
  `src/backend/prealloc/regalloc/call_return_abi.cpp`,
  `src/backend/prealloc/regalloc.cpp`,
  `src/backend/prealloc/regalloc/call_moves.cpp`,
  `src/backend/prealloc/regalloc/value_homes.cpp`,
  `src/backend/prealloc/formal_publications.cpp`,
  `src/backend/prealloc/publication_plans.cpp`, and
  `src/backend/mir/riscv/codegen/object_emission.cpp`.
- `git diff --check -- todo.md` passed.
