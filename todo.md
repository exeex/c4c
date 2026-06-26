Status: Active
Source Idea Path: ideas/open/378_rv64_object_route_20000112_instruction_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Rerun Representative Case

# Current Packet

## Just Finished

Step 2 and Step 3 completed for the first supportable instruction fragment.

Implemented semantic RV64 object-route lowering for same-width integer `ZExt`
casts where operand and result resolve to prepared GPR scalar homes. The
accepted shape publishes the prepared GPR copy/no-op directly from value-home
facts and keeps pointer, same-width `SExt`, source stack-home, and destination
stack-home adjacent shapes fail-closed.

Focused object-emission coverage now proves `i32 -> i32` `ZExt` from `t0` to
`s2` emits the expected GPR copy and serializes as an RV64 object, with the
fail-closed adjacent shapes preserving rejection diagnostics.

## Suggested Next

Execute Step 5 representative rerun for `src/20000112-1.c` using the RV64 GCC
C torture backend allowlist. Confirm the prior first unsupported instruction
fragment (`%t8 = bir.zext i32 %t7 to i32`) is no longer the first blocker, then
record the new first unsupported shape if the representative still fails.

## Watchouts

- Do not reopen idea 369 terminator-fragment lowering or CFG reconstruction.
- Do not treat diagnostic-only changes, allowlist edits, expectation rewrites,
  or named-case shortcuts as capability progress.
- The implementation deliberately excludes pointer `ZExt` and non-GPR homes;
  keep any future extension semantic and backed by prepared type/home facts.
- The next proof should be the representative progress script, not another
  object-emission-only run, unless the representative exposes a regression in
  the focused lowering.

## Proof

Delegated proof command run exactly:

```sh
( cmake --build build --target c4cll backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure ) > test_after.log 2>&1
```

Result: passed. The supervisor-selected proof is sufficient for this focused
implementation and object-emission coverage packet.

Evidence path:

- `test_after.log`
