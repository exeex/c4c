Status: Active
Source Idea Path: ideas/open/319_aarch64_hfa_aggregate_argument_runtime.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate And Classify Residuals

# Current Packet

## Just Finished

Step 4 - Validate And Classify Residuals classified the post-Step-2 first bad
fact without changing implementation or tests.

Classification: the remaining `f128_transport address is not printable`
failure is outside idea 319's AArch64 HFA/aggregate argument classification
and lowering owner. It belongs to the AArch64 `fp128` transport machine-node
addressability/printing surface reached after the argument ABI paths have
already handed off successfully.

Concrete evidence:

- The accepted post-Step-2 proof in `test_before.log` builds successfully and
  runs the exact focused 11-test command with 10 passing. The only failure is
  `c_testsuite_aarch64_backend_src_00204_c`, after semantic/prepared handoff,
  at the AArch64 machine-node printer:
  `cannot print AArch64 machine node family=f128_transport opcode=f128_transport:
  f128 memory transport address is not printable`.
- Generated LLVM for `00204.c` shows the original fixed HFA corruption has
  advanced: `fa_hfa11` is defined as `define void @fa_hfa11(float %p.a.hfa0)`
  and the caller passes a scalar `float` lane instead of `ptr byval(...)`.
- Mixed fixed-parameter long-double HFA calls now expand through `fp128` lanes,
  for example `fa3` receives `%struct.hfa32` as `fp128 %p.c.hfa0, fp128
  %p.c.hfa1`, and `fa_hfa31` through `fa_hfa34` use scalar `fp128` lane
  parameters.
- Callee-side variadic HFA materialization now reaches aggregate `va_arg`
  helper form in generated LLVM, including `%struct.hfa31` through
  `%struct.hfa34` `va_arg` operations in `myprintf`.
- Caller-side variadic HFA direct calls in `stdarg` no longer use unsupported
  array payload operands such as `[1 x float] alignstack(8)` or `[N x fp128]
  alignstack(16)`; long-double HFA variadic calls are emitted as scalar
  `fp128` lanes to `myprintf(ptr, ...)`.
- A focused semantic BIR dump for `stdarg` succeeds through direct variadic
  call lowering, confirming the current first bad fact is later than the
  Step-2 HFA/aggregate semantic ABI owner.
- The local code surfaces naming this later failure are f128 transport/carrier
  surfaces, not HFA argument classification surfaces:
  `src/backend/mir/aarch64/codegen/instruction.cpp` names
  `InstructionFamily::F128Transport`/`MachineOpcode::F128Transport`,
  `src/backend/prealloc/special_carriers.cpp` handles f128 carrier facts, and
  `src/backend/mir/aarch64/codegen/calls.cpp` contains f128 call-boundary move
  handling.

## Suggested Next

Hand the residual to a separate AArch64 `f128_transport` machine-printer /
addressability owner, or have the plan owner split/activate a follow-up idea
for that owner if no suitable active owner exists. Do not expand idea 319 into
generic f128 transport printing: idea 319's argument ABI objective has
advanced to the next backend surface.

## Watchouts

- Preserve the fixed HFA lane repair, the caller-side variadic HFA lane repair,
  and the callee-side `llvm.va_arg.aggregate` helper handoff when opening the
  follow-up f128 transport work.
- Do not reintroduce `[N x float] alignstack(...)` or `[N x fp128]
  alignstack(...)` variadic HFA payloads as a workaround for the printer
  failure.
- Do not special-case `00204.c`, `stdarg`, `myprintf`, one HFA struct, one
  output line, one register, or one stack slot while addressing the residual.
- The separate global data emission symptom still exists in generated assembly
  (`hfa11:` and related HFA globals contain the deferred data comment). Do not
  mix global initializer emission into this packet unless the fixed HFA
  argument path reaches runtime and globals become the first bad fact.
- Preserve prior-owner guardrails for scalar ALU immediate materialization,
  va_start helper text, frame adjustment, stack-offset spelling, runner
  behavior, timeout behavior, proof-log policy, and c-testsuite expectations.

## Proof

Focused proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_aarch64_(target_instruction_records|machine_printer|instruction_dispatch)|backend_cli_dump_(bir|prepared_bir)_(00204_stdarg_(semantic|prepared)_handoff|focus_(function_filters|block_entry)_00204)|c_testsuite_aarch64_backend_src_00204_c)$'
```

Accepted post-Step-2 baseline in `test_before.log`: build succeeded; 11 tests
ran; 10 passed. The only failing test is
`c_testsuite_aarch64_backend_src_00204_c`, now failing after semantic/prepared
handoff at the later AArch64 `f128_transport` machine-node printer blocker
described above.

This todo-only Step 4 packet did not rerun the proof command, so no new
`test_after.log` was written.
