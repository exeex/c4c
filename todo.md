Status: Active
Source Idea Path: ideas/open/372_rv64_object_route_frame_slot_address_call_args.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Frame-Slot Address Call Arguments

# Current Packet

## Just Finished

Executed Plan Step 1 by auditing prepared call-plan and frame-slot address
materialization facts for `src/20000217-1.c` and `src/va-arg-13.c`.

`src/20000217-1.c` is the first in-scope Step 2 implementation target. The
current object-route diagnostic is `unsupported_instruction_fragment`. In
`main`, call block `0` inst `2` calls `showbug` with two GPR ABI arguments:
arg0 goes to `a0`, arg1 goes to `a1`, and both have
`arg.source_selection=local_frame_address_materialization`. The selected
frame slots are `#4` at stack offset `0` for `%lv.x` and `#5` at stack offset
`2` for `%lv.y`; address-materialization metadata records
`kind=frame_slot`, `address_space=default`, `tls_global=no`,
`tls_address_space=no`, and materialization offsets `0` and `2`. The function
frame plan for `main` records `frame_size=12`, `frame_alignment=4`,
`has_dynamic_stack=no`, and `fixed_slots_use_fp=no`.

`src/va-arg-13.c` currently stops before this idea's implementation owner with
`unsupported_param_home`, which belongs to
`ideas/open/374_rv64_object_route_non_register_param_homes.md`. Its prepared
facts still show later frame-slot address call arguments in `test`: block `0`
inst `9` passes `%t7` to `dummy` through `a0` from selected slot `#6` offset
`24`, and block `0` inst `16` passes `%t14` through `a0` from selected slot
`#7` offset `32`. Both are `arg.source_selection=frame_slot_address` with
`missing_frame_slot_arg_publication=yes` and
`missing_frame_slot_arg_kind=frame_slot_address`. The function frame plan for
`test` records `frame_size=80`, `frame_alignment=8`,
`has_dynamic_stack=no`, and `fixed_slots_use_fp=no`.

Audit artifacts:

- `build/agent_state/372_step1_20000217-1.semantic-bir.txt`
- `build/agent_state/372_step1_20000217-1.prepared-bir.txt`
- `build/agent_state/372_step1_20000217-1.runner.log`
- `build/agent_state/372_step1_20000217-1.case.log`
- `build/agent_state/372_step1_20000217-1.audit.txt`
- `build/agent_state/372_step1_va-arg-13.semantic-bir.txt`
- `build/agent_state/372_step1_va-arg-13.prepared-bir.txt`
- `build/agent_state/372_step1_va-arg-13.dummy-prepared-bir.txt`
- `build/agent_state/372_step1_va-arg-13.runner.log`
- `build/agent_state/372_step1_va-arg-13.case.log`
- `build/agent_state/372_step1_va-arg-13.audit.txt`

## Suggested Next

Execute Plan Step 2 by adding the first supportable frame-slot address GPR
call-argument path for the `20000217-1.c` shape:
`arg.source_selection=local_frame_address_materialization`,
default-address-space non-TLS frame-slot materialization, static frame plan,
known frame-slot id/offset, and GPR ABI destination. Materialize the selected
stack-slot address into the prepared destination register before the call;
do not load or pass the slot payload.

## Watchouts

- Do not implement pointer-value local-memory or frame-slot payload-value call
  argument support in this route; ideas 368 and 373 already own those.
- `va-arg-13.c` is currently blocked earlier by
  `unsupported_param_home`; do not absorb non-register parameter homes,
  aggregate `va_arg`, byval homes, terminators, or source-shape shortcuts into
  Step 2.
- The address path must materialize the stack-slot address selected by the
  prepared call plan, not load the payload stored in that slot.
- Add fail-closed tests for missing slot/materialization metadata, dynamic
  stack or unsupported frame base, non-default/TLS address materialization,
  non-GPR ABI destination, out-of-range final offset, and payload-value call
  selections.

## Proof

Audit/probe commands only; no root `test_after.log` was written.

```sh
./build/c4cll --dump-bir --target riscv64-linux-gnu --mir-focus-function main tests/c/external/gcc_torture/src/20000217-1.c > build/agent_state/372_step1_20000217-1.semantic-bir.txt
./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu --mir-focus-function main tests/c/external/gcc_torture/src/20000217-1.c > build/agent_state/372_step1_20000217-1.prepared-bir.txt
./build/c4cll --dump-bir --target riscv64-linux-gnu --mir-focus-function test tests/c/external/gcc_torture/src/va-arg-13.c > build/agent_state/372_step1_va-arg-13.semantic-bir.txt
./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu --mir-focus-function test tests/c/external/gcc_torture/src/va-arg-13.c > build/agent_state/372_step1_va-arg-13.prepared-bir.txt
./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu --mir-focus-function dummy tests/c/external/gcc_torture/src/va-arg-13.c > build/agent_state/372_step1_va-arg-13.dummy-prepared-bir.txt
tmp=$(mktemp); printf '%s\n' src/20000217-1.c > "$tmp"; CASE_TIMEOUT_SEC=20 ALLOWLIST="$tmp" scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/372_step1_20000217-1.runner.log; rc=$?; cp build/rv64_gcc_c_torture_backend/src_20000217-1.c/case.log build/agent_state/372_step1_20000217-1.case.log 2>/dev/null || true; rm -f "$tmp"; exit $rc
tmp=$(mktemp); printf '%s\n' src/va-arg-13.c > "$tmp"; CASE_TIMEOUT_SEC=20 ALLOWLIST="$tmp" scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/372_step1_va-arg-13.runner.log; rc=$?; cp build/rv64_gcc_c_torture_backend/src_va-arg-13.c/case.log build/agent_state/372_step1_va-arg-13.case.log 2>/dev/null || true; rm -f "$tmp"; exit $rc
```

The two one-case runner commands exited nonzero as expected for this audit:
`20000217-1.c` remains at `unsupported_instruction_fragment`, and
`va-arg-13.c` remains at `unsupported_param_home`. `git diff --check` passed.
