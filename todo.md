Status: Active
Source Idea Path: ideas/open/385_rv64_ev64_insn_d_riscv_length_prefix.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Run Backend Regression And Decide Closure

# Current Packet

## Just Finished

Step 5 ran the final backend regression proof and found idea 385
closure-ready from the executor side.

Backend proof:

- Command:
  `ctest --test-dir build -L backend -j --output-on-failure > test_after.log 2>&1`
- Result: passed.
- `test_after.log` reports:
  `100% tests passed, 0 tests failed out of 326`.

Acceptance criteria status:

- Step 1 proved LLVM RISC-V objdump consumes a standalone `0x3f` low-seven-bit
  8-byte candidate as one unknown instruction.
- Commit `11b36c5f Align EV64 insn.d with RISC-V length prefix` implemented
  the c4c `.insn.d` prefixed field layout.
- Step 4 artifacts under `build/agent_state/385_step4/` prove the committed
  encoder emits canonical bytes `3f0320140b040300`, c4c objdump/c4c-as
  roundtrips those bytes exactly, and LLVM shows one 8-byte unknown row at
  address `0` with the next decoded row at `8` and no address `4` split row.
- The final backend regression suite is green after the implementation and
  proof commits.

Conclusion: active runbook evidence is exhausted and idea 385 is ready for
supervisor regression-guard comparison/baseline handling and plan-owner close
decision.

## Suggested Next

Supervisor should run its regression-guard comparison against `test_before.log`
and then route the plan-owner close decision for idea 385 if the guard passes.

## Watchouts

- Preserve the source idea's scope: EV64 `.insn.d` length-prefix layout only.
- Do not implement unrelated RV64 object-route lowering or upstream LLVM
  decoder-table support.
- Keep the existing seven-operand `.insn.d` surface stable; this plan moves
  fields in the binary encoding and does not add/remove operands.
- c4c-objdump intentionally does not decode the old `major`-in-low-bits layout.
- Executor did not compare or roll `test_before.log`; supervisor owns
  regression guard comparison and baseline roll-forward.

## Proof

Command:

```bash
ctest --test-dir build -L backend -j --output-on-failure > test_after.log 2>&1
```

Result: passed. Proof log: `test_after.log`.
