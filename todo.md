Status: Active
Source Idea Path: ideas/open/100_aarch64_00204_stdarg_hfa_runtime_repair.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Prove The AArch64 Targeted Cases

# Current Packet

## Just Finished

Repaired AArch64 outgoing stack argument lifetime for prepared-frame-slot
sources without weakening supported-path expectations. The chosen invariant is:
call-boundary outgoing stack writes publish the x16-relative outgoing argument
slot as their destination lifetime while preserving the selected prepared frame
source bytes as uses. For byval register-lane transports, both register and
stack destinations read from the call plan's selected prepared source byte
range; a byte may come from a wider prepared frame store. HFA/FPR overflow
stack lanes are materialized from the semantic prepared source value instead of
trusting stale selected frame slots.

Focused contract coverage now checks a non-HFA stack argument whose regalloc
home differs from the selected prepared frame source slot. The representative
HFA overflow and f128 HFA route tests are covered by the proof subset.

## Suggested Next

Execute `plan.md` Step 6 / Step 7 proof and close-readiness validation.

## Watchouts

- Preserve the current source idea; the reviewer found the repair still aligned
  with `ideas/open/100_aarch64_00204_stdarg_hfa_runtime_repair.md`.
- Do not repair by padding HFA float lanes, changing `00204.c` expectations, or
  matching the literal testcase output shape.
- The packet must cover prepared-frame-slot sources copied into outgoing stack
  arguments, including at least one non-HFA stack argument case and the current
  HFA overflow representative.

## Proof

Exact delegated focused proof run and green before broader validation:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_prepare_liveness|backend_aarch64_call_boundary_owner|backend_codegen_route_aarch64_(byval_payload_8_to_13_stack_overflow|byval_payload_9_to_14_stack_overflow|hfa_global_payload_call_boundary|f128_hfa_global_payload_call_boundary)|c_testsuite_aarch64_backend_src_(00032|00182|00204)_c)$' | tee test_after.log`

Result: 10/10 tests passed, including `00204.c`, `00032.c`, and `00182.c`.

Supervisor broader backend before/after guard:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: both `test_before.log` and `test_after.log` report 178/179 passing
with the same known `backend_aarch64_instruction_dispatch` failure. Regression
checker reported no new failures and no pass-count loss, but exited nonzero
because the broad pass count did not strictly increase. Canonical broad logs:
`test_before.log` and `test_after.log`.
