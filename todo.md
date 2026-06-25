Status: Active
Source Idea Path: ideas/open/373_rv64_object_route_frame_slot_value_call_args.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Frame-Slot Value Call Arguments

# Current Packet

## Just Finished

Executed Plan Step 1 by auditing prepared call-plan, value-home, and frame-slot
facts for `src/20000121-1.c`.

Artifacts:

- `build/agent_state/373_step1_20000121-1.semantic-bir.txt`
- `build/agent_state/373_step1_20000121-1.prepared-bir.txt`
- `build/agent_state/373_step1_20000121-1.runner.log`
- `build/agent_state/373_step1_20000121-1.case.log`
- `build/agent_state/373_step1_20000121-1.audit.txt`

Concrete facts:

- Source selection is in `doit` at the first `big` call:
  `bir.call void big(i64 %t1)` with `call_arg_source index=0
  encoding=register source_value=%t1`.
- `%t1` is the scalar payload produced by `bir.sext i8 %t0 to i64`; `%t0`
  comes from the pointer-value local load from `%p.id`.
- Prepared value-home facts place `%t1` at `value_id=5`,
  `kind=stack_slot slot_id=2 offset=16`.
- Prepared storage facts encode `%t1` as `frame_slot`, `bank=gpr`,
  `spill_slot=slot#2+stack16`, `width=1`.
- Frame-slot facts for `doit` publish `frame_size=24`,
  `frame_alignment=8`, and `slot #2 object_id=2 func=doit name=%t1
  source_kind=regalloc.spill_slot type=i64 size=8 align=8 offset=16`.
- Prepared call-plan facts for `doit` block 0 instruction 2 publish
  `arg index=0 value_bank=gpr source_encoding=frame_slot source_value_id=5
  source_slot=#2 source_stack_offset=16 source_bank=gpr` and ABI destination
  `dest_placement=gpr:call_argument#0/w1 dest_reg=a0 dest_bank=gpr`.
- The same argument records `arg.source_selection=frame_slot_value`,
  `selection_source_value=%t1`, `selection_source_home=stack_slot`,
  `selection_source_size=8`, `selection_source_align=8`,
  `missing_frame_slot_arg_publication=yes`,
  `missing_frame_slot_arg_source_materializes_address=no`, and
  `missing_frame_slot_arg_may_emit_local_payload=no`.
- Current object-route runner diagnostic remains:
  `unsupported_instruction_fragment: BIR instruction requires unsupported RV64
  object lowering`; the prepared call-plan marker narrows the boundary to the
  missing scalar frame-slot-value argument publication before the first
  `big(%t1)` call.

## Suggested Next

Execute Plan Step 2 by adding focused RV64 object lowering for a direct
same-module scalar GPR call argument whose prepared call plan has
`source_encoding=frame_slot`, whose value home is a known frame slot with
encodable final stack offset, and whose ABI destination is a GPR argument
register. The positive fixture should mirror `%t1` from slot `#2` offset 16 to
`a0`; negative fixtures should keep frame-slot addresses, missing slots,
non-GPR destinations, bad width/alignment, and out-of-frame offsets fail-closed.

## Watchouts

- Do not include frame-slot address call arguments; those belong to idea 372.
- Do not reopen pointer-value local-memory support from closed idea 368.
- Do not handle aggregate `va_arg`, byval homes, terminators, or source-shape
  shortcuts in this route.
- Progress must be based on prepared value-home and call-plan metadata.
- The semantic BIR source annotation is still `encoding=register`; use the
  prepared call-plan `source_encoding=frame_slot` and value-home/storage facts
  as the authority for object emission.
- This packet did not implement code. The direct `--codegen asm` probe exits
  cleanly; the failing boundary is the RV64 object runner path.

## Proof

Audit-only packet; no `test_after.log` was written.

Commands run:

```sh
./build/c4cll --dump-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000121-1.c > build/agent_state/373_step1_20000121-1.semantic-bir.txt
./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu --mir-focus-function doit tests/c/external/gcc_torture/src/20000121-1.c > build/agent_state/373_step1_20000121-1.prepared-bir.txt
tmp=$(mktemp); printf '%s\n' src/20000121-1.c > "$tmp"; CASE_TIMEOUT_SEC=20 ALLOWLIST="$tmp" scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/373_step1_20000121-1.runner.log; cp build/rv64_gcc_c_torture_backend/src_20000121-1.c/case.log build/agent_state/373_step1_20000121-1.case.log; rm -f "$tmp"
```

The one-case object runner result is the expected current failure:
`total=1 passed=0 failed=1`.
