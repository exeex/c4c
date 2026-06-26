Status: Active
Source Idea Path: ideas/open/388_rv64_object_route_variadic_va_end_boundary.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture The Prepared `va_end` Boundary

# Current Packet

## Just Finished

Step 1 captured the prepared/BIR boundary for unresolved `llvm.va_end.p0` in
`tests/c/external/gcc_torture/src/va-arg-13.c` without implementing a fix.

Evidence:

- Existing object/link boundary remains unresolved references to
  `llvm.va_end.p0` at `.text+0xac` and `.text+0xdc` in
  `build/agent_state/386_step3_va-arg-13.case.log`.
- Semantic BIR for focused function `test` has two direct calls:
  `bir.call void llvm.va_end.p0(ptr %lv.state.8)` at entry instruction 11
  after the first `dummy(ptr %t7)` call, and again at entry instruction 17
  after the second `dummy(ptr %t14)` call.
- Prepared BIR preserves the same calls at block 0 instruction indexes 11 and
  17. Both are prepared call plans with
  `wrapper_kind=direct_extern_fixed_arity`, `indirect=no`,
  `variadic_fpr_arg_register_count=0`, `callee=llvm.va_end.p0`, and one GPR
  argument.
- For both `va_end` calls, arg0 is `%lv.state.8` / value id 10, sourced from
  callee-saved register `s1`, moved to call argument register `a0`, and
  selected through `arg.source_selection=prior_preservation` from the previous
  `dummy` call boundary.
- The prepared variadic entry plan for `test` reports helper resources
  `helpers=[va_start]` and only two `va_start` helper operands at block 0
  inst 3 and inst 12. There is no prepared `va_end` helper operand; current
  preparation classifies `llvm.va_end.p0` as an ordinary fixed-arity extern
  call.
- Likely next owner: RV64 object emission around
  `fragment_for_prepared_instruction` in
  `src/backend/mir/riscv/codegen/object_emission.cpp`, specifically the
  prepared variadic helper path (`fragment_for_prepared_variadic_va_start`,
  `fragment_for_prepared_variadic_va_arg_aggregate`,
  `diagnose_unsupported_prepared_variadic_helper_fragment`) before fallback to
  ordinary `fragment_for_prepared_call`. Step 2 needs to decide whether
  `llvm.va_end.p0` should become a prepared variadic helper/no-op route or a
  narrower unsupported diagnostic instead of flowing to extern-call emission.

## Suggested Next

Execute Step 2: classify the route for `llvm.va_end.p0`: either add an exact
prepared variadic-helper/no-op contract for RV64 object emission, or emit a
narrow unsupported diagnostic before ordinary extern-call lowering.

## Watchouts

- Keep idea 386 closed; do not reopen frame-slot-address call argument
  lowering under this plan.
- Keep idea 387 sret work out of scope.
- Do not hide unresolved `llvm.va_end.p0` with fake symbols or linker
  workarounds.
- The current prepared variadic entry plan does not publish `va_end` helper
  operands, so a no-op route must either be justified by exact callee/argument
  facts on the direct extern call or add preparation support for a real
  `va_end` helper classification.
- Do not generalize ordinary `direct_extern_fixed_arity` intrinsic calls unless
  the route is guarded to `llvm.va_end.p0` and the one-argument va_list shape.

## Proof

Ran the supervisor-selected evidence command:

```bash
mkdir -p build/agent_state && { echo 'Step 1 evidence capture for idea 388'; echo 'Existing case boundary:'; rg -n 'llvm\.va_end|va_end|undefined reference|\.text\+' build/agent_state/386_step3_va-arg-13.case.log || true; echo '--- dump-bir ---'; ./build/c4cll --dump-bir --target riscv64-linux-gnu --mir-focus-function test tests/c/external/gcc_torture/src/va-arg-13.c > build/agent_state/388_step1_va_arg_13.bir.log; rg -n 'llvm\.va_end|va_end|call|test' build/agent_state/388_step1_va_arg_13.bir.log | sed -n '1,220p'; echo '--- dump-prepared-bir ---'; ./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu --mir-focus-function test tests/c/external/gcc_torture/src/va-arg-13.c > build/agent_state/388_step1_va_arg_13.prepared_bir.log; rg -n 'llvm\.va_end|va_end|prepared-call|call|test' build/agent_state/388_step1_va_arg_13.prepared_bir.log | sed -n '1,260p'; } > test_after.log 2>&1
```

Proof log: `test_after.log`

Evidence logs:

- `build/agent_state/388_step1_va_arg_13.bir.log`
- `build/agent_state/388_step1_va_arg_13.prepared_bir.log`
