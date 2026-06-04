Status: Active
Source Idea Path: ideas/open/100_aarch64_00204_stdarg_hfa_runtime_repair.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Implement The Semantic ABI Repair

# Current Packet

## Just Finished

Completed Step 4 of `plan.md`: implemented the semantic byval aggregate
source-selection repair for AArch64 register-lane byval arguments whose payload
comes from same-module global aggregate loads.

Implementation details:

- `select_byval_payload_lane_load_source` now recognizes
  `.global.aggregate.load.` payload lanes alongside the existing
  `.array.aggregate.load.` lanes.
- Load-selected byval lanes now carry the payload load value name and
  instruction index into `PreparedCallArgumentSourceSelection`, so the prepared
  `ByvalRegisterLane` source identifies the actual loaded payload byte instead
  of the stale aggregate carrier.
- Store-based byval source fallback remains unchanged.
- The focused AArch64 global byval register-lane source-selection contract now
  passes without weakening the assertion.

## Suggested Next

Proceed to the next packet by tracing the remaining `00204.c` runtime mismatch
after the direct global byval source-selection repair. The next trace should
separate any remaining direct aggregate argument/return corruption from later
stdarg/HFA variadic-entry issues before implementing another repair.

## Watchouts

- Do not downgrade `00204.c` expectations or mark it unsupported.
- Do not special-case `00204.c` or its literal output shape.
- Keep `00032.c` and `00182.c` visible as AArch64 guard cases.
- Treat narrow probes as ABI-fact probes, not testcase-shaped shortcuts.
- `00204.c` still fails as a runtime mismatch in the delegated proof scope; do
  not treat this Step 4 repair as a full testcase fix.
- The focused contract and guard tests are green, so the remaining mismatch is
  outside this owned source-selection slice.
- The next packet should not special-case `00204.c`; continue tracing ABI facts
  and keep the stdarg/HFA work separated from direct aggregate source repair.

## Proof

Ran:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|c_testsuite_aarch64_backend_src_00204_c|c_testsuite_aarch64_backend_src_00032_c|c_testsuite_aarch64_backend_src_00182_c)$'
```

Result: exit code 8 because the delegated proof scope still includes the known
`c_testsuite_aarch64_backend_src_00204_c` runtime mismatch.
`backend_prepare_frame_stack_call_contract` passed, both guard tests
`c_testsuite_aarch64_backend_src_00032_c` and
`c_testsuite_aarch64_backend_src_00182_c` passed, and
`c_testsuite_aarch64_backend_src_00204_c` remains failing. Canonical executor
proof log: `test_after.log`.
