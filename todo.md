Status: Active
Source Idea Path: ideas/open/373_rv64_object_route_frame_slot_value_call_args.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Admit Scalar Frame-Slot Value Call Arguments

# Current Packet

## Just Finished

Repaired Plan Step 2 by admitting the real scalar frame-slot-value call
argument shape from `src/20000121-1.c`.

The previous positive fixture missed two real prepared facts: `doit` uses a
function-specific frame plan (`frame_size=24`) while module `stack_layout`
reports `frame_size=1`, and `%t1` has a stack-slot value home with slot/offset
only while size/alignment are published on the call-argument source selection.
The RV64 object path now uses the function frame plan for object emission,
accepts the call-selection size/alignment facts for frame-slot-value call
arguments, and can publish scalar integer cast results into prepared stack
homes before the call reload.

The one-case representative now passes:

- runner result: `total=1 passed=1 failed=0`
- runner log: `build/agent_state/373_step2_20000121-1.repair-runner.log`
- copied case log: `build/agent_state/373_step2_20000121-1.repair-case.log`

## Suggested Next

Proceed to Plan Step 4 lifecycle decision for idea 373, using the focused
backend proof and passing `src/20000121-1.c` representative as closure
evidence.

## Watchouts

- Do not include frame-slot address call arguments; those belong to idea 372.
- Do not handle aggregate `va_arg`, byval homes, terminators, or source-shape
  shortcuts in this route.
- The repair deliberately stays semantic: it consumes prepared frame-plan,
  value-home, call-selection, and frame-slot facts rather than matching the
  representative name or source shape.
- The temporary debug coordinate used during diagnosis was `doit` block `0`
  instruction `1`; no debug diagnostic remains in the code.

## Proof

Delegated Step 2 proof ran exactly as requested and wrote `test_after.log`.
Result: `3/3` passed.

Command run:

```sh
cmake --build build --target c4cll backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test &&
ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure > test_after.log
```

Additional representative check:

```sh
tmp=$(mktemp); printf '%s\n' src/20000121-1.c > "$tmp"; CASE_TIMEOUT_SEC=20 ALLOWLIST="$tmp" scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/373_step2_20000121-1.repair-runner.log; rc=$?; cp build/rv64_gcc_c_torture_backend/src_20000121-1.c/case.log build/agent_state/373_step2_20000121-1.repair-case.log 2>/dev/null || true; rm -f "$tmp"; exit $rc
```

Result: `total=1 passed=1 failed=0`.
