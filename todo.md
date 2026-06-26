Status: Active
Source Idea Path: ideas/open/395_rv64_object_route_instruction_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Iterate Or Split Remaining Families

# Current Packet

## Just Finished

Step 4 - Iterate Or Split Remaining Families classified the next five
`unsupported_instruction_fragment` representatives after the completed
`src/20000223-1.c` repair. The reusable next target-lowering family is
represented by `src/20000402-1.c`: prepared scalar integer unsigned division
fragments, including `%t2 = bir.udiv i64 %t0, %t1` and `%t3 = bir.udiv i64
85899345920, 2147483648`, reaching `fragment_for_prepared_binary()` without an
RV64 object lowering rule for `bir.udiv`.

The other four representatives are adjacent buckets rather than the same
family: `src/20000403-1.c` has a same-module call argument immediate `4096`
that exceeds the current signed-12-bit call-immediate path;
`src/20000622-1.c` has frame-slot call arguments with
`missing_frame_slot_arg_publication=yes`; `src/20000917-1.c` has sret
aggregate/local-frame-address call shapes; and `src/20001221-1.c` has large
i64 literal materialization/comparison pressure.

## Suggested Next

Delegate an implementation packet owning
`src/backend/mir/riscv/codegen/object_emission.cpp` to lower prepared scalar
`bir.udiv` for RV64 object emission, using `src/20000402-1.c` as the first
representative. Keep the packet semantic: support the binary-op fragment shape
generally for supported integer widths/operand homes, not the testcase
constants.

## Watchouts

- Do not mix `20000403-1.c` into the udiv packet; it is a call-argument
  materialization gap for immediates outside signed-12-bit range.
- Do not mix `20000622-1.c` into the udiv packet; its frame-slot call arguments
  are explicitly marked with missing publication facts.
- Keep aggregate/sret local-frame-address handling from `20000917-1.c` and
  large literal handling from `20001221-1.c` as separate packets unless a later
  proof shows they collapse into the same producer/target fact.

## Proof

Ran the delegated proof exactly. `test_after.log` is the proof log:
`[rv64-gcc-torture] total=5 passed=0 failed=5`, with all five representatives
still reporting `unsupported_instruction_fragment`. Classification artifacts
are under `build/agent_state/395_step4_*`, with the summary in
`build/agent_state/395_step4_instruction_fragment_classification.txt`.
