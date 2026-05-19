Status: Active
Source Idea Path: ideas/open/319_aarch64_hfa_aggregate_argument_runtime.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Owned Argument Path

# Current Packet

## Just Finished

Step 2 - Repair The Owned Argument Path advanced past the localized mixed
fixed-parameter blocker and the variadic HFA `va_arg` load-local blocker while
preserving the fixed HFA lane repair. It now also advances past the variadic
HFA caller-side `stdarg` direct-call semantic blocker.

Implemented facts:

- Fixed non-Apple AArch64 HFA parameters now classify through the HIR-to-LIR
  ABI layout path using the canonical cross-table TypeSpec owner lookup.
- Function signatures and call type suffixes expand fixed float, double, and
  long-double HFA aggregate parameters into scalar FP lanes.
- Same-module direct calls expand fixed HFA aggregate arguments by loading each
  aggregate member lane and passing scalar `float`, `double`, or `fp128` call
  operands instead of `ptr byval(...)`.
- Callee entry materializes expanded fixed HFA lane parameters back into the
  original aggregate local slot so existing field-access lowering can keep
  using aggregate lvalues.
- Mixed fixed-parameter semantic BIR handoff now skips all scalar/function
  pointer parameters when collecting aggregate params, so a later aggregate
  fixed parameter after expanded HFA lanes still materializes correctly.
- Fixed HFA call-argument lane extraction now materializes a local aggregate
  temporary before byte-offset lane loads, avoiding the prior global-memory GEP
  semantic failure in the caller path.
- AArch64 HFA `va_arg` now hands aggregate results to semantic BIR through the
  existing structured `LirVaArgOp`/`llvm.va_arg.aggregate` helper instead of
  emitting raw loads from computed `vr_top + offset` va-list pointers.
- AArch64 variadic HFA caller arguments now lower as scalar FP lanes instead
  of LLVM array payload values such as `[1 x float] alignstack(8)`, so semantic
  BIR direct-call lowering can represent the call ABI lanes directly.
- Focused local coverage was added to `backend_lir_to_bir_notes` for an
  AArch64 direct-call HFA-shaped argument contract: two `float` lanes lower to
  BIR as `F32, F32` call arguments with no byval ABI marker.
- Focused local coverage was added for AArch64 direct variadic HFA-shaped call
  arguments: a `ptr, ...` call with two HFA float lanes lowers as `Ptr, F32,
  F32`, marks the call variadic, and preserves FP/SSE ABI classes without
  byval markers.
- Focused local coverage was added for mixed scalar plus aggregate parameter
  materialization, proving that aggregate params after scalar ABI lanes remain
  available to local-memory lowering.
- Focused local coverage was added for AArch64 HFA aggregate `va_arg` helper
  handoff, proving `llvm.va_arg.aggregate` carries the aggregate result and the
  result remains storeable as an aggregate local.

Representative generated LLVM evidence for the original first bad fact now
advances past `fa_hfa11(hfa11)`:

```llvm
define void @fa_hfa11(float %p.a.hfa0)
...
%t19 = load %struct.hfa11, ptr @hfa11
%t20 = alloca %struct.hfa11
store %struct.hfa11 %t19, ptr %t20
%t21 = getelementptr i8, ptr %t20, i64 0
%t22 = load float, ptr %t21
call void (float) @fa_hfa11(float %t22)
```

The original localized BIR fact `bir.call void fa_hfa11(ptr byval(size=4,
align=4) %t19)` is no longer the first owning failure after this repair.
The intermediate `fa3`, `fa4`, and caller `arg` semantic BIR blockers exposed
by mixed expanded HFA lanes plus remaining aggregate parameters have also
advanced. The later `myprintf` variadic HFA `va_arg` raw load-local blocker
has also advanced. The later `stdarg` variadic HFA direct-call semantic
blocker has also advanced.

## Blocked On

The exact focused proof command now reaches the AArch64 machine-node printer
and fails at a later f128 transport surface:

- Failing test: `c_testsuite_aarch64_backend_src_00204_c`
- Failure phase: backend machine-node printer after semantic/prepared handoff
- Latest failure: `cannot print AArch64 machine node family=f128_transport
  opcode=f128_transport: f128 memory transport address is not printable`
- Triggering source shape: later HFA long-double (`fp128`) transport after the
  fixed HFA, callee-side variadic HFA, and caller-side variadic HFA semantic
  ABI paths have all lowered far enough to reach machine printing.
- Representative generated LLVM after the caller-side variadic HFA repair:

```llvm
define void @stdarg()
...
%t404 = getelementptr [28 x i8], ptr @.str63, i64 0, i64 0
...
%t407 = getelementptr i8, ptr %t406, i64 0
%t408 = load double, ptr %t407
...
call void (ptr, ...) @myprintf(ptr %t404,
                               double %t408, double %t410,
                               double %t412, double %t414, ...)
```

The previous array-shaped HFA variadic call operands are gone from the
caller-side path. Long-double HFA lanes now reach later `fp128` transport
lowering/printing, which is outside the direct semantic-call blocker repaired
in this packet.

## Suggested Next

Use the next packet to repair or classify the AArch64 `f128_transport`
machine-printer addressability blocker reached after HFA argument lowering.
Preserve fixed HFA lanes, variadic HFA caller lanes, and the
`llvm.va_arg.aggregate` callee-side helper handoff; do not fold this into
generic runtime or expectation work.

## Watchouts

- The separate global data emission symptom still exists in generated assembly
  (`hfa11:` and related HFA globals contain the deferred data comment). Do not
  mix global initializer emission into this packet unless the fixed HFA
  argument path reaches runtime and globals become the first bad fact.
- Preserve prior-owner guardrails for scalar ALU immediate materialization,
  va_start helper text, frame adjustment, stack-offset spelling, runner
  behavior, timeout behavior, proof-log policy, and c-testsuite expectations.
- Long-double HFAs are now classified as AAPCS64 HFA lanes (`fp128`) for the
  fixed path; do not silently drop that coverage while investigating the
  variadic blocker.

## Proof

Focused proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)|backend_cli_dump_(bir|prepared_bir)_(00204_stdarg_(semantic|prepared)_handoff|focus_(function_filters|block_entry)_00204)|c_testsuite_aarch64_backend_src_00204_c)$'
```

Result in `test_after.log`: build succeeded; 11 tests ran; 10 passed. The only
failing test is `c_testsuite_aarch64_backend_src_00204_c`, now failing after
semantic/prepared handoff at the later AArch64 `f128_transport` machine-node
printer blocker described above.
