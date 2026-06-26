Status: Active
Source Idea Path: ideas/open/391_rv64_variadic_prologue_save_area_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture The Variadic Save-Area Boundary

# Current Packet

## Just Finished

Step 1: Capture The Variadic Save-Area Boundary is complete for
`tests/c/external/gcc_torture/src/va-arg-13.c`.

Captured current semantic/prepared/disassembly evidence under
`build/agent_state/391_step1_va-arg-13.*`.

Exact boundary:

- Incoming payload source: `main` calls `test(456, 1234)` with prepared call
  fact `arg index=1 value_bank=gpr source_encoding=immediate
  source_literal=1234 ... dest_reg=a1`.
- Variadic callee shape: prepared `test` has `variadic_entry=yes`,
  `named_params=1`, `named_gp=1`, so the first variadic GPR payload is the
  incoming call argument after named `fmt`, i.e. RV64 ABI register `a1`.
- Save-area destination: prepared variadic entry plan creates
  `overflow_area required=yes base_slot=#12 base_stack_offset=72 align=8` and
  `va_list_layout` with one `overflow_arg_area` field. Both `va_start` helpers
  target `dst_va_list=%lv.state.8:stack_slot:slot=#13:offset=72` with
  `dst_va_list_addr=%lv.state.8:register:reg=s1`.
- Object slots/registers: stack layout has object #12
  `__rv64_variadic_overflow_area_base` at slot #12 offset 72 size 0, object #13
  `__rv64_variadic_va_start_destination_6` at slot #13 offset 72 size 8, and
  `%lv.state.8` value id 10 in callee-saved register `s1`. The later call
  objects are `%t7` value id 15 slot #9 stack+48 and `%t14` value id 19 slot
  #10 stack+56, with their address-exposed local objects at slots #6 and #7.
- Prepared publication gap: `register_save_area required=no`,
  `saved_gp=<unknown>`, `gp_slot=<unknown>`, and
  `--- prepared-block-entry-publications ---` is empty. Store-source
  publications exist for local pointer copies only (`%lv.state`,
  `%t7.memcpy.copy.0`, `%t14.memcpy.copy.0`); there is no prepared prologue or
  save-area publication fact from incoming `a1` to base_slot #12 / offset 72.
- Object/disassembly symptom: clang spills incoming variadic registers in
  `test`, including `sd a1, 0x8(s0)`, before setting the `va_list` pointer to
  that area. c4c sets `s1 = sp+0x48` / slot #13 and stores the pointer value
  there, but never stores incoming `a1=1234` to that backing area before
  `dummy` loads through the `va_list`.

Likely owner: RV64 object-route variadic-entry prologue / prepared variadic
helper emission should publish incoming variadic GPR payloads into the prepared
overflow/save area, guarded by explicit prepared variadic entry facts. The
prepared-call `FrameSlotAddress` publication/copy route from idea 390 is not
the remaining owner.

## Suggested Next

Step 2 should classify the guarded save-area publication route: which prepared
variadic entry facts are required to copy incoming post-named GPR argument
payloads, such as `a1`, into the overflow/save area at base_slot #12 before
`va_start` exposes `slot #13 offset 72`.

## Watchouts

- Keep ideas 386, 387, 388, 389, and 390 out of scope unless new evidence
  proves this boundary belongs to one of those routes.
- Do not hard-code `va-arg-13.c`, `test`, `dummy`, registers, stack offsets,
  or the abort branch.
- Address or pointer materialization alone is not proof that the backing
  variadic payload save area was published.
- The current prepared facts identify the variadic entry and `va_start`
  destination but do not yet identify a concrete saved-GPR publication table;
  Step 2 should fail closed for absent or ambiguous `saved_gp` / save-area
  facts rather than inferring payload authority from `a1` alone.

## Proof

Evidence-only packet. No implementation files or tests were changed, so the
delegated backend proof was not rerun. The current build proof remains the idea
390 close gate:

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
- Result: PASS, 326/326 before and 326/326 after, no new failures.

Evidence paths:

- `build/agent_state/391_step1_va-arg-13.semantic-bir.log`
- `build/agent_state/391_step1_va-arg-13.prepared-bir.log`
- `build/agent_state/391_step1_va-arg-13.c4c-disasm.log`
- `build/agent_state/391_step1_va-arg-13.clang-disasm.log`
- `build/agent_state/391_step1_va-arg-13.case.log`
- `build/agent_state/391_step1_va-arg-13.qemu-strace.log`
- `test_after.log` remains the root proof log from the prior representative
  failure / close-gate state.
