Status: Active
Source Idea Path: ideas/open/378_rv64_object_route_20000112_instruction_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit First Unsupported Instruction Fragment

# Current Packet

## Just Finished

Step 1 audit completed for `src/20000112-1.c`.

Fresh proof still fails at `unsupported_instruction_fragment`. Prepared/object
route evidence identifies the first unsupported ordinary instruction shape as a
same-width promoted boolean cast in `main`, block `block_3`, instruction index
3:

```text
%t8 = bir.zext i32 %t7 to i32
```

Semantic facts from the prepared dump:

- Opcode/type shape: `ZExt` cast, operand type `i32`, result type `i32`.
- Operand home: `%t7` value_id=21, GPR register home `t0`.
- Destination home: `%t8` value_id=22, GPR register home `s2`.
- Publication facts: a before-instruction move bundle exists at
  `block_index=2 instruction_index=3`, moving value_id 21 (`%t7`) to
  value_id 22 (`%t8`) as a register-to-register consumer move into `s2`.
- Surrounding chain: `%t6 = bir.ne i32 %t5, 0`, `%t7 = bir.xor i32 %t6, 1`,
  `%t8 = bir.zext i32 %t7 to i32`, `%t9 = bir.ne i32 %t8, 0`, then fused
  branch condition `compare=ne i32 %t8, 0`.

Audit probes under `build/agent_state/378_step1_20000112_probe*.log` confirm
that the reduced first-call path passes, while the reduced inverted second-call
path fails with the same `unsupported_instruction_fragment` and contains the
same `zext i32 -> i32` shape.

## Suggested Next

Execute Step 2: implement semantic RV64 object-route lowering for same-width
integer `ZExt` casts where operand and result are already prepared GPR scalar
values, treating the cast as a no-op/copy publication instead of rejecting it.
Keep non-integer, widening, unsupported-width, and unsupported-home cast shapes
fail-closed.

## Watchouts

- Do not reopen idea 369 terminator-fragment lowering or CFG reconstruction.
- Do not treat diagnostic-only changes, allowlist edits, expectation rewrites,
  or named-case shortcuts as capability progress.
- Keep any implementation scoped to semantic instruction-fragment lowering and
  fail-closed adjacent shapes.
- The implementation target is not the testcase string or `main` block name;
  it is the semantic same-width integer `ZExt` prepared cast shape. The current
  RV64 cast lowering rejects `result_bits <= source_bits`, so the no-op
  same-width case needs an explicit accepted path.

## Proof

Delegated proof command run exactly:

```sh
mkdir -p build/agent_state && printf '%s\n' 'src/20000112-1.c' > build/agent_state/378_step1_20000112.allowlist.txt && ALLOWLIST=build/agent_state/378_step1_20000112.allowlist.txt STOP_ON_FAILURE=1 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log 2>&1
```

Result: exited nonzero as expected for this audit packet, with
`total=1 passed=0 failed=1` and the current
`unsupported_instruction_fragment` failure preserved in `test_after.log`.

Evidence paths:

- `test_after.log`
- `build/agent_state/378_step1_20000112.runner.log`
- `build/agent_state/378_step1_20000112.allowlist.txt`
- `build/agent_state/378_step1_20000112.prepared_bir.log`
- `build/agent_state/378_step1_20000112.semantic_bir.log`
- `build/agent_state/378_step1_20000112_probe_results.log`
- `build/agent_state/378_step1_20000112_probe_second_call_invert.prepared_bir.log`
