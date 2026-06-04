Status: Active
Source Idea Path: ideas/open/100_aarch64_00204_stdarg_hfa_runtime_repair.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Prove The AArch64 Targeted Cases

# Current Packet

## Just Finished

Completed Step 5 follow-up implementation of `plan.md`: repaired direct
AArch64 byval register-lane call-boundary materialization in
`src/backend/mir/aarch64/codegen/calls.cpp`.

Implemented facts:

- Aggregate register-lane call-boundary moves now resolve prepared byval
  payload byte slots back through prepared addressing to the same-block
  `store_local` value.
- For global aggregate payload lanes, the call boundary emits direct AArch64
  global byte loads into the outgoing ABI GPR lane and packs multi-byte lanes
  with `orr`, instead of consuming stale prepared stack scratch bytes.
- Missing trailing ABI lane bytes are treated as zero padding during direct
  register materialization, so 1..16 byte direct byval register-lane payloads
  no longer depend on stale stack padding.
- Final AArch64 for `arg` now materializes calls such as `fa_s1`, `fa_s2`,
  and following direct byval register-lane calls from globals (`s1`, `s2`,
  etc.) into `x0`/successive ABI lanes before the call.
- Added focused backend route proof
  `backend_codegen_route_aarch64_byval_global_payload_call_boundary`, which
  compiles a tiny global two-byte aggregate by-value call and checks final
  AArch64 assembly for payload-byte loads from `global_pair` into the outgoing
  `x0` lane before `bl take_pair`, while forbidding stack reloads into `x0`.

## Suggested Next

Repair the next non-call-boundary aggregate materialization families exposed by
`00204.c`: direct >16-byte byval/address argument materialization, direct HFA
argument lanes, and direct aggregate/sret return stores.

## Watchouts

- Do not downgrade `00204.c` expectations, mark it unsupported, or special-case
  its literal output shape.
- Keep `00032.c` and `00182.c` visible as guard cases.
- Do not start with stdarg/HFA-vararg repair: the current signature proves
  direct >16-byte byval/address, direct HFA arguments, and direct
  aggregate/sret returns are still wrong before the variadic sections execute.
  Stdarg and HFA-varargs remain downstream until those direct paths are correct.
- The fixed direct integer byval register-lane calls still leave stale stack
  publications in the assembly, but the call boundary no longer consumes them.
  Do not treat those stale stores as proof of failure unless a later consumer
  still reloads them into the ABI argument registers.

## Proof

Ran:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_byval_global_payload_call_boundary|backend_prepare_frame_stack_call_contract|c_testsuite_aarch64_backend_src_00204_c|c_testsuite_aarch64_backend_src_00032_c|c_testsuite_aarch64_backend_src_00182_c)$'
```

Result: exit code 8. The focused byval call-boundary route proof, the existing
prepared contract, and both guard cases passed; 5 tests ran, 4 passed, and
`c_testsuite_aarch64_backend_src_00204_c` remains the only failing test in this
scope. `00204.c` improved: the direct integer byval
argument section now prints correctly through the 1..16 byte register-lane
cases, then still fails at the >16-byte byval/address case plus direct HFA
argument, return/sret, stdarg, and HFA-vararg materialization. Canonical
executor proof log: `test_after.log`.
