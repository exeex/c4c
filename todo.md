Status: Active
Source Idea Path: ideas/open/653_stack_carried_pointer_source_publication_materialization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement The Narrow Stack-Carried Pointer Rule

# Current Packet

## Just Finished

Step 3, `Implement The Narrow Stack-Carried Pointer Rule`: attempted the narrow
`%t6` prepared/RV64 rule and stopped at the exact producer-authority boundary.
Evidence is recorded in
`build/agent_state/653_step3_stack_carried_pointer_rule/summary.md`.

- Focused representative:
  `tests/c/external/gcc_torture/src/20140828-1.c`.
- Focused RV64 backend object route:
  `build/agent_state/653_step3_stack_carried_pointer_rule/src_20140828-1.c/case.log`
  still reports `[RV64_BACKEND_RUNTIME_MISMATCH]` with c4c aborting.
- Prepared dump:
  `build/agent_state/653_step3_stack_carried_pointer_rule/src_20140828-1.c/prepared.txt`
  still shows `%t6` value id `19` as
  `preservation_source=stack_slot:slot#16:value#19`, not as an explicit
  `%lv.a.0 + 2` computed local-frame pointer source.
- Disassembly:
  `build/agent_state/653_step3_stack_carried_pointer_rule/src_20140828-1.c/c4c.disasm.txt`
  still has no pre-call materialization/store of `sp + 6` into slot `#16`; the
  branch still reloads `ld t4,8(sp)`.
- Blocker: the producer cannot currently prove source identity `%lv.a.0 + 2`,
  materialized value identity `%t6`, and selected stack home `slot#16+stack8`
  in one explicit prepared fact. RV64 must remain fail-closed rather than
  infer that fact from stack offsets, source spelling, or assembly shape.

## Suggested Next

Executor packet: add the prepared producer authority or fail-closed diagnostic
in call-preservation planning for stack-carried pointer values whose source
home should be a materializable `PointerBasePlusOffset` local-frame address.
The first positive shape is `%t6` value id `19` at slot `#16`, source
`%lv.a.0 + 2`; the negative shape is the current stack-slot-only preservation
source, which must keep RV64 from materializing the branch operand.

## Watchouts

- Do not reopen RV64 terminator-fragment admission from idea 645.
- Do not infer pointer freshness or materialization from stack offsets, final
  assembly shape, source spelling, local names, diagnostics, testcase identity,
  runtime outcomes, or pass/fail accounting.
- `loop-2e.c` now passes the direct runtime runner; do not use `%t23` as the
  first failing runtime proof unless a later packet identifies a still-red
  focused owner.
- The `%t6` row has explicit branch-stack-load authority and call-preserve
  metadata, but no explicit `%t6` source materialization/publication fact.
  Preserve RV64's fail-closed behavior for missing, stale, ambiguous, and
  mismatched producer facts; do not make RV64 infer the source from slot
  offsets, final assembly shape, or testcase identity.
- The current `call_plans.cpp` preservation endpoint for `%t6` is built from
  the selected stack home, so it records the source as `slot#16` rather than
  the semantic pointer source `%lv.a.0 + 2`. The producer-side change must
  carry both identities without weakening ordinary stack-slot preservation.

## Proof

`test_after.log`: `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_'`.

Result: build completed and the delegated backend subset remains red with 32
failed tests. The failed-test list matches `test_before.log`, so this blocked
producer-authority packet did not introduce a new backend failure set.

Additional focused proof: the RV64 backend object route for
`src/20140828-1.c` still aborts at runtime; log path is
`build/agent_state/653_step3_stack_carried_pointer_rule/src_20140828-1.c/case.log`.
