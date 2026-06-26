Status: Active
Source Idea Path: ideas/open/388_rv64_object_route_variadic_va_end_boundary.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Lowering Or Diagnostic Route

# Current Packet

## Just Finished

Step 2 classified the narrow RV64 object-route strategy for prepared
`llvm.va_end.p0` without implementing a fix.

Selected route:

- Implement narrow no-op lowering for `llvm.va_end.p0` in RV64 object emission
  before fallback to ordinary extern-call emission.
- Guard this route on the current direct extern prepared call facts rather than
  requiring new prepared helper classification first. The evidence shows
  `prepared_variadic_entry_helper_kind_for_callee` recognizes `va_start`,
  `va_copy`, and `va_arg*`, but not `llvm.va_end.p0`; the prepared variadic
  entry plan for `va-arg-13.c` publishes `helpers=[va_start]` only. Adding
  `VaEnd` to the prepared helper enum/plan is broader than needed for the
  immediate RV64 object-route boundary.
- The no-op must be semantic, not testcase-shaped: it should recognize the
  prepared call shape for the variadic lifecycle intrinsic and emit an empty
  fragment, not a fake external symbol or linker workaround.

Exact guard facts for the next packet:

- `bir::CallInst` has `callee == "llvm.va_end.p0"`.
- The indexed prepared call plan exists for the same block/instruction.
- The call and prepared plan agree on direct callee name.
- `wrapper_kind == DirectExternFixedArity`.
- The call is not indirect and has no `callee_value`; the prepared call plan is
  not indirect and has no `indirect_callee`.
- The prepared call plan has no `memory_return` and no
  `outgoing_stack_argument_area`.
- The call has exactly one argument and the prepared call plan has exactly one
  argument.
- The argument is the prepared va_list pointer argument already materialized by
  call-boundary facts. In the representative evidence both calls use
  `%lv.state.8` / value id 10, sourced from callee-saved register `s1` and
  moved to `a0` by the before-call bundle. The implementation should not guard
  on those specific ids/registers, only on the one-argument prepared call
  contract.

Likely owned files/functions for the next implementation packet:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- Add a narrow helper near existing variadic fragments, e.g.
  `fragment_for_prepared_variadic_va_end(...)`, returning an empty
  `RiscvEncodedFragment` only when the exact guard facts hold.
- Call that helper from `fragment_for_prepared_instruction` after the existing
  `va_start` / `va_arg_aggregate` variadic checks and before
  `fragment_for_prepared_call`.
- Focused backend/object-route tests chosen by the supervisor should prove
  that `llvm.va_end.p0` leaves no relocation/external call while adjacent calls
  remain unchanged.

## Suggested Next

Execute Step 3: add focused RV64 backend/object-route coverage for the selected
`llvm.va_end.p0` no-op behavior before implementing the lowering.

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
- Unsupported variants that remain out of scope: `va_copy`, scalar `va_arg`,
  aggregate `va_arg`, changes to `va_start`, ordinary extern calls, indirect
  calls, direct extern variadic calls, memory-return calls, outgoing stack
  argument calls, malformed `llvm.va_end.p0` calls with missing or multiple
  arguments, mismatched prepared call-plan callee names, and any fake symbol or
  linker allowlist workaround.

## Proof

Ran the supervisor-selected Step 2 classification command:

```bash
mkdir -p build/agent_state && { echo 'Step 2 route classification for idea 388'; echo '--- clang tool availability ---'; command -v c4c-clang-tool-ccdb || true; command -v c4c-clang-tool || true; echo '--- object emission symbols ---'; c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/mir/riscv/codegen/object_emission.cpp build/compile_commands.json 2>/dev/null | rg 'fragment_for_prepared|variadic|call|extern|instruction' || true; echo '--- relevant text anchors ---'; rg -n 'llvm\.va_end|va_end|va_start|va_arg|fragment_for_prepared_instruction|fragment_for_prepared_call|fragment_for_prepared_variadic|diagnose_unsupported_prepared_variadic_helper|direct_extern_fixed_arity|DirectExternFixedArity' src/backend/mir/riscv/codegen/object_emission.cpp src/backend/prealloc src/backend/mir src/backend/bir | sed -n '1,260p'; echo '--- Step 1 prepared evidence ---'; rg -n 'call block_index=0 inst_index=(11|17)|callee=llvm\.va_end\.p0|helpers=|arg index=0 .*source_value_id=10' build/agent_state/388_step1_va_arg_13.prepared_bir.log | sed -n '1,220p'; } > test_after.log 2>&1
```

Proof log: `test_after.log`

Evidence logs:

- `build/agent_state/388_step2_route_classification.log`
- Step 1 evidence reused from
  `build/agent_state/388_step1_va_arg_13.prepared_bir.log`
