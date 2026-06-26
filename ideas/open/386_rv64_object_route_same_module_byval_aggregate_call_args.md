# RV64 Object Route Same-Module Byval Aggregate Call Arguments

Status: Open
Type: Target ABI follow-up
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
Split From: `ideas/closed/384_prepared_global_symbol_memory_access_publication.md`
Related: `ideas/closed/370_rv64_object_route_byval_aggregate_param_homes.md`

## Goal

Lower or precisely route RV64 object-route same-module calls whose prepared
callsite contains a byval aggregate/address argument.

## Why This Exists

Idea 384 published prepared global-symbol memory-access facts for
`src/20030914-2.c` and moved the representative past the old global-data
boundary. The new first failure is in `main`, block `entry`, inst `36`:

```text
%t1 = bir.call i32 f(ptr byval(size=72, align=4) %t0, i32 4660)
```

The prepared callsite classifies the call as `wrapper=same_module`,
`callee=f`, `arg0 bank=aggregate_address from=register:s1 to=none`, and
`arg1 bank=gpr from=immediate:4660 to=a1`. RV64 object emission currently
rejects it with:

```text
unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering
```

Closed idea 370 admitted byval aggregate parameter homes, but this boundary is
later: moving an aggregate/address argument at a same-module callsite, using
prepared callsite and ABI facts. It is not another prepared global-symbol
publication problem.

## In Scope

- Audit the prepared callsite, byval, aggregate-address, and same-module
  wrapper facts available for `src/20030914-2.c`.
- Define the first supportable RV64 call-argument movement for an aggregate
  address source already materialized in a register.
- Lower the supported call only from explicit prepared callsite, ABI placement,
  aggregate-address, and byval facts.
- Preserve precise unsupported diagnostics for missing byval metadata,
  unsupported aggregate source locations, dynamic layout, unsupported outgoing
  stack requirements, indirect callees, or ambiguous ABI placement.
- Add focused RV64 object-emission or prepared call-contract tests for the
  supported same-module byval aggregate/address argument and fail-closed
  unsupported shapes.
- Rerun `src/20030914-2.c` and document whether it passes or advances to
  aggregate `va_arg`, terminator, control-flow, non-register parameter-home, or
  another distinct owner.

## Out of Scope

- Reopening prepared global-symbol memory-access publication from idea 384.
- Reopening byval aggregate entry parameter homes from closed idea 370.
- Aggregate `va_arg` helper lowering; use
  `ideas/open/371_rv64_object_route_aggregate_va_arg_helper_lowering.md`.
- Non-register parameter homes; use
  `ideas/open/374_rv64_object_route_non_register_param_homes.md`.
- General indirect-call, external-call, multi-block CFG, or broad RV64 call
  lowering unrelated to this same-module byval aggregate/address argument
  boundary.
- Inferring callsite ABI placement, aggregate size, or source address from C
  source syntax, testcase names, raw diagnostic text, or assumed stack layout.

## Acceptance Criteria

- Focused backend tests prove the selected same-module byval aggregate/address
  call-argument behavior or a more precise unsupported boundary.
- Existing prepared call-contract, byval parameter-home, global-symbol
  publication, and RV64 object-emission coverage remains green.
- `src/20030914-2.c` advances beyond the current call instruction boundary
  because the object route consumes prepared callsite facts, or records a
  clearly routed next unsupported owner.
- Raw or incomplete callsite facts remain fail-closed with diagnostics that
  identify the missing prepared fact family.

## Reviewer Reject Signals

- Reject named-case-only handling for `src/20030914-2.c`, callee `f`, global
  `gs`, register `s1`, immediate `4660`, or aggregate size `72`.
- Reject lowering that reconstructs byval aggregate layout, call argument
  placement, or outgoing stack state from source syntax, raw BIR text, or
  target-side assumptions instead of prepared callsite and ABI facts.
- Reject expectation downgrades, allowlist filtering, unsupported-contract
  weakening, or diagnostic-only renames claimed as same-module byval call
  progress.
- Reject broad RV64 call-lowering rewrites that do not prove this aggregate
  address/byval argument shape and nearby fail-closed unsupported shapes.
- Reject routes that move aggregate `va_arg`, non-register parameter homes,
  global data publication, or unrelated CFG work into this idea unless the
  prepared callsite contract directly requires it.
- Reject retaining the exact `unsupported_instruction_fragment` boundary for
  the representative behind renamed helpers or moved diagnostics.

## Suggested First Step

Audit the prepared callsite record and the current RV64 `CallInst` emission
path for the representative. Record which facts already identify the
same-module wrapper, callee, argument banks, byval size/alignment, source
register, destination placement, clobbers, and outgoing stack requirements.
