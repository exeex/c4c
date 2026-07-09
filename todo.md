Status: Active
Source Idea Path: ideas/open/648_rv64_call_arg_frame_slot_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Post-656 Representative Call Evidence

# Current Packet

## Just Finished

Completed Step 1 by refreshing post-656 representative evidence for
`src/20000722-1.c` under
`build/agent_state/648_post656_call_evidence/`.

The current prepared call fact is `bar:entry` instruction 10, same-module call
to `foo`, argument 0. It explicitly selects
`arg.source_selection=local_frame_address_materialization` for
`selection_source_value=%lv._clit_`, `selection_source_slot=#0`,
`selection_source_stack_offset=0`, size 8, align 8. The source home is
`gpr:callee_saved#1/w1`, `source_reg=x21` / RV64 `s2`; the ABI destination is
`gpr:call_argument#0/w1`, `dest_reg=x0` / RV64 `a0`.

The prepared move bundle still records
`reason=call_arg_register_to_register` for the same value into `x0`/`a0`.
Post-656 RV64 object emission now succeeds, and fresh `llvm-objdump` shows the
representative call setup as `mv s2, sp` followed by `mv a0, s2` before the
`foo` relocation. This packet did not confirm the stale pre-656 `mv a0, s1`
shape for the representative row.

Compared with the focused green
`riscv64_call_arg_local_frame_address_materialization` test, the prepared
source-selection contract is the same explicit local-frame-address shape. The
focused green text-route CTest emits direct `addi a0, sp, 0`, while the focused
object route captured here also uses a two-step object pattern (`mv s1, sp`;
`mv a0, s1`).

Current owner: RV64 consumption of
`arg.source_selection=local_frame_address_materialization`, specifically the
text-route direct materialization versus object-route materialize-into-source
register plus ABI-copy boundary. The refreshed evidence does not identify a
distinct upstream owner, ABI register-choice issue, local-memory admission
issue, or string-label pointer policy issue.

## Suggested Next

Execute Step 2 by tracing the representative prepared call source selection
through `src/backend/mir/riscv/codegen/prepared_call_emit.cpp` and
`src/backend/mir/riscv/codegen/object_emission.cpp`. Determine why the focused
text route emits direct `addi a0, sp, ...` while the object route materializes
the selected frame-slot address into the source callee-saved register and then
copies that register into `a0`.

## Watchouts

- Do not rely on pre-656 `mv a0, s2` evidence; refresh the representative row.
- The refreshed representative object evidence is `mv s2, sp` then
  `mv a0, s2`; do not carry forward the stale `mv a0, s1` note unless a fresh
  probe reintroduces it.
- The representative text asm output exits 0 but does not reach the `bar` call
  setup before the `foo` label; use the object disassembly for current emitted
  representative call evidence.
- Do not reopen idea 656 local-memory policy or string-label pointer admission.
- Do not infer frame-slot address materialization from source spelling, stack
  offsets, final assembly, testcase identity, or diagnostic text.
- Do not special-case `src/20000722-1.c`, `%lv._clit_`, `foo`, `bar`, `s1`,
  `s2`, or `a0`.
- Do not edit expectations, unsupported markers, allowlists, timeouts,
  runtime-comparison policy, or pass/fail accounting.

## Proof

No build/ctest proof was required for this diagnostic-only packet. Ran focused
prepared-BIR, semantic BIR, MIR summary/trace, RV64 asm, RV64 object, and
`llvm-objdump` diagnostics for `src/20000722-1.c`; also captured focused green
prepared/asm/object comparison evidence. Evidence lives under
`build/agent_state/648_post656_call_evidence/`, with summary at
`build/agent_state/648_post656_call_evidence/summary.md`. Did not create or
overwrite `test_after.log`.
