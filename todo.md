Status: Active
Source Idea Path: ideas/open/372_rv64_object_route_frame_slot_address_call_args.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Admit Frame-Slot Address GPR Call Arguments

# Current Packet

## Just Finished

Executed Plan Step 2 by adding focused RV64 object lowering for prepared
`LocalFrameAddressMaterialization` GPR call arguments. The admitted path uses
the prepared call-plan destination register plus selected frame-slot
address-materialization metadata to emit `addi <arg-reg>, sp, <offset>` before
the call. It rejects invalid selected address facts instead of falling back to
passing the payload/source register.

Focused tests now cover the positive two-argument frame-slot-address call
shape and fail-closed neighbors: payload-value selection, missing slot or
materialization metadata, absent materialization records, non-default/TLS
materialization, non-GPR destination, dynamic stack, fixed-slot frame-pointer
layouts, and out-of-range final offsets.

The required one-case rerun for `src/20000217-1.c` still exits nonzero:
`total=1 passed=0 failed=1`. The case log remains at the generic
`unsupported_instruction_fragment` diagnostic. Additional prepared probes saved
for this packet show the remaining visible out-of-scope owner in `showbug`:
pointer-value local accesses are followed by scalar compare-result lowering
for `sge` and `trunc i1 -> i16`; the current object-route scalar binary path
only handles compare results for `eq`/`ne`.

Artifacts:

- `test_after.log`
- `build/agent_state/372_step2_20000217-1.runner.log`
- `build/agent_state/372_step2_20000217-1.case.log`
- `build/agent_state/372_step2_20000217-1.main-prepared-bir.txt`
- `build/agent_state/372_step2_20000217-1.showbug-prepared-bir.txt`
- `build/agent_state/372_step2_20000217-1.showbug-semantic-bir.txt`

## Suggested Next

Run supervisor/plan-owner route review for idea 372. The focused Step 2
capability is implemented and proved, but the representative cannot yet prove
through to the frame-slot address call in file-level object mode because
`showbug` exposes a separate scalar compare-result owner (`sge` result plus
`trunc i1 -> i16`).

## Watchouts

- Keep this route scoped to frame-slot address call arguments; do not absorb
  scalar compare-result lowering, non-register parameter homes, aggregate
  `va_arg`, byval homes, terminators, or source-shape shortcuts.
- `va-arg-13.c` remains blocked earlier by `unsupported_param_home`, which
  belongs to `ideas/open/374_rv64_object_route_non_register_param_homes.md`.
- The emitter now treats explicit selected source metadata as authoritative:
  a selected local frame address must materialize as an address, and unsupported
  selected source kinds must not silently fall back to ordinary register moves.

## Proof

Delegated focused proof passed and wrote `test_after.log`:

```sh
cmake --build build --target c4cll backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test &&
ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure > test_after.log
```

Additional one-case check was run and saved under `build/agent_state/`:

```sh
tmp=$(mktemp); printf '%s\n' src/20000217-1.c > "$tmp"; CASE_TIMEOUT_SEC=20 ALLOWLIST="$tmp" scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/372_step2_20000217-1.runner.log; rc=$?; cp build/rv64_gcc_c_torture_backend/src_20000217-1.c/case.log build/agent_state/372_step2_20000217-1.case.log 2>/dev/null || true; rm -f "$tmp"; exit $rc
```

Result: `total=1 passed=0 failed=1`, with the residual classified above.
