Status: Active
Source Idea Path: ideas/open/627_pointer_stack_result_call_policy.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish Pointer Result Destination/Home Facts

# Current Packet

## Just Finished

Completed Step 3 by confirming that the existing prepared call-result
producer already publishes complete pointer result destination/home authority
for an in-scope RV64 representative row before object emission. No producer
code changed.

Evidence command:
`build/c4cll --target riscv64-linux-gnu --dump-prepared-bir tests/c/external/gcc_torture/src/20011113-1.c`

Evidence artifact:
`/tmp/c4c627_step3_evidence/20011113-1.prepared`

Confirmed rows:

- `foo` callsite `block=0 inst=16`, same-module wrapper, result
  `destination_value_id=12`, source `source_storage=register`,
  `source_reg=a0`, `source_bank=gpr`, `source_placement=gpr:call_result#0/w1`,
  destination `destination_storage=stack_slot`, `destination_slot=#29`,
  `dest_stack_offset=112`, `dest_spill_slot=slot#29+stack112`,
  late publication `late_publication=yes late_source_register=yes`.
- Matching prepared home for that result is `home %t8 value_id=12
  kind=stack_slot slot_id=29 offset=112`; matching storage/home summary is
  `storage %t8 value_id=12 encoding=frame_slot bank=gpr
  spill_slot=slot#29+stack112 width=1 slot_id=#29 stack_offset=112`.
- Stack object/frame-slot authority for the same destination is
  `object #29 func=foo name=%t8 type=ptr size=8 align=8` and `slot #29
  offset=112 size=8 align=8`.
- `baz` callsite `block=0 inst=19` independently shows the same complete
  shape for result `destination_value_id=26`, source `a0`, destination
  `destination_slot=#38`, `dest_stack_offset=112`, and matching home
  `home %t17 value_id=26 kind=stack_slot slot_id=38 offset=112`.

Producer conclusion:

- Existing `PreparedCallResultPlan` and late-publication carriers are
  sufficient for Step 4. The producer publishes callsite identity, result
  value id, source GPR register and width, destination stack slot id/offset,
  pointer object size, and a matching prepared value home.
- No actual producer publication bug was found, so implementation files,
  tests, expectations, unsupported markers, allowlists, and the RV64 consumer
  were not touched in this packet.

## Suggested Next

Step 4 should be a narrow RV64 consumer admission packet in
`src/backend/mir/riscv/codegen/object_emission.cpp` plus focused backend
coverage. Replace the current pointer-only rejection in the stack-slot
call-result branch with fail-closed admission that requires the Step 3 facts:
ordinary same-module direct call, pointer return/result type, GPR register
source, one-register source/destination width, destination stack slot id and
offset, matching prepared value home, RV64 pointer store width of 8 bytes, and
the existing absolute stack-slot offset helper.

## Watchouts

- The Step 4 packet should reject missing or mismatched destination homes
  through the existing unsupported same-module call diagnostic path.
- Do not infer authority from source filenames, final assembly, ABI register
  names alone, or aggregate shape.
- Keep adjacent buckets out of this idea: scalar frame-slot argument
  publication, byval/outgoing aggregate stack transport, FPR, variadic,
  library/runtime, local/global pointer memory traffic, expectation changes,
  unsupported markers, allowlists, timeout, or accounting work.
- `20011113-1.c` still contains an adjacent outgoing aggregate/byval argument
  stack-copy bucket; use only the explicit prepared call-result facts above as
  Step 4 authority.

## Proof

Command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed, 347/347 backend tests. Proof log: `test_after.log`.
