Status: Active
Source Idea Path: ideas/open/353_aarch64_local_formal_frame_slot_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add focused scalar formal/local coverage and repair formal-to-local slot publication

# Current Packet

## Just Finished

Step 2 added focused AArch64 scalar formal-to-local coverage and repaired
formal-to-local slot publication for register-backed fixed formals.

Changes:

- Added dispatch coverage for a scalar fixed formal copied into a local frame
  slot, loaded before a call, loaded again after the call, and returned through
  a local-derived add. The fixture keeps the incoming formal in a register home
  while the store-local destination is a distinct frame slot, so stale stack
  reloads fail the test instead of passing accidentally.
- Taught AArch64 memory lowering to build a selected store when the destination
  is a frame-slot memory operand, the stored value home is a register, and the
  stored value's storage-plan row is frame-slot-backed. The selected store now
  consumes the formal's register value home in the correct scalar view.
- Added a narrow dispatch fallback for fixed-formal local store publication
  when ordinary memory lowering recognizes the store but does not emit a store
  instruction. The fallback uses the recorded scalar register and prepared
  frame-slot address, preserving the existing same-register formal publication
  behavior while making later local loads see the incoming value.

Representative progress:

- `00176` no longer stops at the stale formal/local-slot read timeout. In the
  generated `partition`, the incoming formal is now published before relevant
  local loads, including `str w0, [sp]`, `str w0, [sp, #8]`, and after
  call-preserved scalar recovery `str w21, [sp, #12]`.
- The representative advances to a runtime mismatch instead of timing out.

## Suggested Next

Execute Step 3/3 against the new first bad fact in `00176`: global indexed
array snapshot/writeback lowering in `swap` appears to write array elements
from uninitialized high stack snapshot slots such as `[sp, #264]` and
`[sp, #268]`, producing the corrupted final array after the formal/local
publication issue is repaired.

## Watchouts

- Keep idea 352's repaired `.LBB90_6`/`.LBB90_7` branch/label path intact.
- Do not special-case `00176`, `partition`, one local name, one stack offset,
  one formal register, or one emitted instruction sequence.
- Do not broaden into variadic, byval, HFA, aggregate writeback, or broad frame
  layout without fresh first-bad-fact evidence.
- The fixed-formal repair intentionally avoids changing BIR local-slot
  creation, stack layout, expectation files, or runner policy.
- The new `swap` symptom looks outside the fixed-formal local-slot publication
  boundary; keep any follow-up semantic and avoid testcase-shaped matching.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_operand_resolution|backend_prepare_frame_stack_call_contract|backend_prepare_stack_layout|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00176_c)$' | tee test_after.log`

Result: build passed; 5/6 tests passed. The five backend guardrails passed.
`c_testsuite_aarch64_backend_src_00176_c` no longer timed out, but failed with
a runtime output mismatch after corrupting the final sorted array. Proof log:
`test_after.log`.

Supervisor broader guard:

`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed, 141/141.
