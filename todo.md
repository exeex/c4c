Status: Active
Source Idea Path: ideas/open/363_rv64_object_parameter_home_coverage.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Unsupported Parameter Homes

# Current Packet

## Just Finished

Completed `plan.md` Step 1 audit for the RV64
`unsupported_param_home` boundary behind `src/20030914-2.c`.

The first rejected prepared parameter-home class is a callee formal byval
aggregate pointer whose authoritative prepared home is a stack slot:

- representative prepared dump command:
  `build/c4cll -I tests/c/external/gcc_torture --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20030914-2.c`
- BIR formal: `bir.func @f(ptr byval(size=72, align=4) %p.pa, i32 %p.pb) -> i32`
- rejected formal home: `home %p.pa value_id=0 kind=stack_slot slot_id=0 offset=0`
- accepted scalar peer: `home %p.pb value_id=1 kind=register reg=t0`
- producer storage/object facts expose this as a byval home slot:
  `object #18 func=f name=%p.pa source_kind=byval_param type=ptr size=72 align=4 address_exposed=yes requires_home_slot=yes permanent_home_slot=yes`
- aggregate payload lane copies are present as separate register homes
  (`%lv.param.p.pa.aggregate.param.copy.*`), so the missing object-route
  contract is not "no payload facts"; it is the formal byval parameter address
  being represented by a stack-slot home rather than an accepted GPR formal
  home.

The RV64 consumer check is `prepared_param_homes_supported()` in
`src/backend/mir/riscv/codegen/object_emission.cpp`. It iterates
`function.params`, calls `gpr_register_number_for_value(...)`, and rejects any
formal whose prepared home cannot be classified by `gpr_register_number_for_home`.
AST lookup confirmed the direct caller is `prepared_function_to_object_function()`.
That path emits the current broad diagnostic:
`unsupported_param_home: RV64 object route requires all parameters in supported GPR homes`.

Nearby same-family case inspected: `src/920908-1.c` also has non-GPR formal
homes in its prepared dump (`%ret.sret` as `kind=stack_slot` plus local stack
temporaries), but it is not the clean first boundary for this idea because the
current RV64 admission order reaches the variadic aggregate-helper diagnostic
there before parameter-home support. Keep sret stack-slot formals separate from
the byval aggregate formal unless Step 2 deliberately defines them together.

## Suggested Next

Implement Step 2 as a focused contract/diagnostic packet for RV64 formal
parameter homes:

- add focused coverage that distinguishes ordinary GPR formals from byval
  aggregate formals whose parameter address home is a prepared stack slot
- either define the semantic object-route contract for lowering the byval
  stack-slot formal home, or replace the broad `unsupported_param_home`
  boundary with a precise byval-stack-home diagnostic
- preserve the existing `prepared_param_homes_supported()` GPR behavior and do
  not make object admission infer support from testcase names or source shape

Suggested narrow proof command:

```sh
cmake --build build --target backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure > test_after.log
```

## Watchouts

- Treat `src/20030914-2.c` as a representative signal, not an implementation
  key.
- Do not conflate the callee formal byval stack-slot home (`%p.pa`) with caller
  aggregate-address call-argument transport. The representative caller has
  prepared aggregate-address argument facts, but the observed rejection happens
  while admitting callee function parameters.
- Do not silently admit every stack-slot formal. `src/920908-1.c` shows sret and
  local stack homes nearby; those may need a separate contract from byval
  aggregate parameter homes.
- Do not reopen scalar vararg, `va_arg`, `va_copy`, or variadic helper work
  from idea 362 and its follow-ups.
- Do not broaden into vector ABI, unrelated FPR ABI, globals, data sections, or
  broad object-route rewrites.
- Progress must be based on structured parameter-home classes; diagnostic
  renames, expectation rewrites, skips, and testcase-shaped matching are not
  enough.

## Proof

Ran:

```sh
git diff --check -- todo.md
```

Result: passed.
