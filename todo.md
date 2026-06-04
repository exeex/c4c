Status: Active
Source Idea Path: ideas/open/100_aarch64_00204_stdarg_hfa_runtime_repair.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Prove The AArch64 Targeted Cases

# Current Packet

## Just Finished

Completed Step 5 decomposed direct small-aggregate stack/overflow argument
packet for fixed AArch64 `fa1`/`fa2` calls.

First wrong fact repaired:

- Stack-destination byval register-lane arguments had prepared
  `ByvalRegisterLane` source selections and outgoing stack offsets, but
  `plan_prepared_aggregate_transport` only built aggregate transport metadata
  for register destinations. Final AArch64 lowering therefore reserved the
  outgoing area for `fa1`/`fa2` but emitted no payload stack publication.
- After enabling aggregate transport metadata for stack destinations, lowering
  emitted stack copies, which localized the next wrong fact: the prepared
  source frame slots for global aggregate bytes were stale scratch publications.
- AArch64 stack-destination byval lowering now traces each prepared payload
  byte through the recorded frame-slot stored-value fact and rematerializes
  same-block global byte producers directly into the outgoing stack area before
  the call.
- Added focused route probes
  `backend_codegen_route_aarch64_byval_payload_8_to_13_stack_overflow` and
  `backend_codegen_route_aarch64_byval_payload_9_to_14_stack_overflow`; they
  prove the two helper shapes publish stack overflow bytes before `fa1`/`fa2`.
- Final `00204.c` proof output now shows the direct `Arguments:` lines for
  `fa1` and `fa2` matching expected exactly:
  `stu ABC JKL TUV 456 ghi` and `ABC JKL TUV 456 ghi tuv`.

## Suggested Next

Trace the next remaining `00204.c` ABI family: aggregate/HFA return value and
sret lowering. The direct `Arguments:` section now matches expected; the first
remaining mismatch starts at `Return values:`.

## Watchouts

- Do not downgrade `00204.c` expectations, mark it unsupported, or special-case
  its literal output shape.
- Keep `00032.c` and `00182.c` visible as guard cases.
- This packet repaired fixed direct stack/overflow small-aggregate argument
  materialization only. Return/sret lowering and stdarg/variadic lowering
  remain downstream.
- Stale scratch stack stores may still appear for prepared homes, but consumed
  outgoing ABI stack slots for the repaired byval direct calls are overwritten
  from traced payload bytes before the call.

## Proof

Ran:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_.*|backend_prepare_frame_stack_call_contract|c_testsuite_aarch64_backend_src_(00204|00032|00182)_c)$'
```

Result: exit code 8. Expected for this bounded packet: 21 of 22 selected tests
passed, including all selected backend route tests, the two new byval stack
overflow probes, `backend_prepare_frame_stack_call_contract`, and guard cases
`00032.c`/`00182.c`. `c_testsuite_aarch64_backend_src_00204_c` remains the
only failing test in this scope; the direct `Arguments:` section now matches
expected, and the remaining mismatch starts in return/sret output before later
stdarg/variadic ABI families. Canonical executor proof log: `test_after.log`.
