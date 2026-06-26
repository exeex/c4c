Status: Active
Source Idea Path: ideas/open/382_rv64_object_route_prepared_local_memory_addressing.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Prepared Local-Memory Addressing Lowering

# Current Packet

## Just Finished

Completed `plan.md` Step 3 by verifying that the audited prepared byval
stack-slot pointer-value local-memory lane load shape is already semantically
lowered in the RV64 object route.

No `object_emission.cpp` implementation change was needed. The current helper
path is `fragment_for_prepared_load_local` ->
`prepared_byval_stack_slot_pointer_access_offset` ->
`append_rv64_load_stack_to_register`. That helper validates default address
space, non-volatile pointer-value base, prepared base-plus-offset addressing,
matching prepared value-home and frame-slot metadata, a permanent
`byval_param` stack object, size/align compatibility, in-bounds byte offsets,
frame bounds, and signed-12-bit encodability before emitting the `sp`-relative
load.

The Step 2 fixtures at offsets `0` and `68` exercise that path and remain green
without testcase-shaped code. The same helper participates in
`diagnose_unsupported_prepared_instruction_fragment`, preserving the precise
fail-closed local-memory diagnostic when any required prepared fact is absent.

## Suggested Next

Next packet should run the supervisor-selected Step 4 representative proof for
the prepared RV64 object route.

## Watchouts

- `prepared_byval_stack_slot_pointer_access_offset` is also called by the store
  path, but this packet verified the delegated load shape only.
- Missing byval/home facts can still be rejected before local-memory analysis by
  `unsupported_param_home` or `unsupported_byval_param_home`; those diagnostics
  are intentional and should not be weakened.
- The feasible adjacent local-memory boundary remains offset `69`, because an
  artificial unencodable byval home offset is rejected earlier by stack-frame
  admission rather than by local-memory lowering.

## Proof

Ran the delegated proof command exactly:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'; } > test_after.log 2>&1
```

Result: passed. Build reported `ninja: no work to do`, and CTest reported
`1/1 Test #215: backend_riscv_object_emission .... Passed`. Proof log:
`test_after.log`.
