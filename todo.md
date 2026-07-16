Status: Active
Source Idea Path: ideas/open/841_lir_compact_scalar_abi_leaf_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add the compact scalar authority carrier

# Current Packet

## Just Finished

Completed Step 1 inventory for idea 841. Current scalar authority is still
spread across `LirTypeRef` kind/width/builtin mirrors plus scalar-only op
fields such as `LirBinOp.type_str`, `LirCmpOp.type_str`,
`LirCastOp.from_type`/`to_type`, `LirAbsOp.int_type`,
`LirCallOp.return_type`, `LirCallArg.type_ref`, `LirSelectOp.type_str`,
`LirRet.type_str`, signature return/parameter refs, and ABI leaf uses of
pointer/void. Verifier and printer consume those refs through
`require_module_type_ref`, operation-specific checks, and LLVM rendering.
Raw-BIR compatibility still lowers many scalar rows through
`lower_lir_type`, `lower_scalar_or_function_pointer_type`,
`lower_integer_type`, and exact selected scalar receipt probes.

Selected bounded Step 2 target: add the minimal compact scalar authority
carrier/store/ref for true scalar builtins and wire it first to
`LirBinOp.type_str` as a compatibility mirror owner for integer and floating
binary ops. This target is small enough to prove at the producer/schema,
verifier, printer, and current Raw-BIR compatibility boundary without
reopening the accepted `LirAbsOp` selected-global/i32 receiver row from
`0c44e810ad`.

True scalar rows for this target: integer widths currently represented by
`LirTypeRef::integer(...)` / `LirTypeKind::Integer` and floating builtins
`half`, `float`, `double`, `fp128`, and `x86_fp80` when used by scalar binary
ops. Pointer and void are ABI leaves only, not scalar members of this first
carrier slice.

Excluded families for Step 2: vector types and native vector store/refs,
aggregate structs/unions/arrays/anonymous structs, function refs and call
signature family migration, opaque/runtime text, pointer truthiness and pointer
cast policy, void return/parameter ABI leaves, inline assembly template or
constraint parsing, memory/VA/object/lifetime authority, Raw-BIR receiver
handoffs under 734, and terminal deletion of scalar text escape hatches.

Missing evidence: exact pointer/void ABI-leaf migration consumers remain
separate evidence-needed followups after the scalar carrier exists; no opaque
type is accepted as scalar.

## Suggested Next

Execute Step 2 by adding the compact scalar carrier/ref in the LIR type model
and attaching it to `LirBinOp.type_str` as the selected scalar-only schema
mirror. Keep existing `LirTypeRef` text/kind rendering as compatibility, and
add focused wrong-family rejection for vector, aggregate, function, opaque,
pointer, and void inputs to the selected binary-op scalar carrier.

## Watchouts

Do not implement 734 Raw-BIR receiver work in this idea. Do not assume opaque is
scalar, do not parse rendered text as scalar authority, and do not reopen
accepted receiver rows such as `LirAbsOp` selected-global/i32.

For Step 2, do not migrate `LirCmpOp`, `LirCastOp`, `LirCallOp`,
`LirSelectOp`, returns, signatures, or pointer/void ABI leaves in the same
packet. `LirBinOp` may include both integer and floating scalar opcodes, but
the carrier must reject vector/aggregate/function/opaque/pointer/void families
before later schemas can rely on it.

## Proof

Step 1 inventory proof: `git diff --check`; passed as the documentation-only
packet proof. No `test_after.log` was produced because the delegated proof
command is a diff whitespace check and no implementation/test command was run.

Suggested focused proof for Step 2 after code edits:
`cmake --build build --target frontend_lir_call_type_ref_test && ctest --test-dir build --output-on-failure -R '^frontend_lir_call_type_ref$'`.
