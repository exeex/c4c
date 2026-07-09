Status: Active
Source Idea Path: ideas/open/627_pointer_stack_result_call_policy.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Pointer Stack-Result Evidence

# Current Packet

## Just Finished

Completed Step 1 evidence refresh for the six idea 627 representative rows.
All six reach semantic BIR as ordinary same-module pointer-result calls and all
six currently fail RV64 object emission at:
`unsupported_call_abi: RV64 object route requires supported ordinary same-module
call ABI/result lowering`.

Prepared evidence:

- `tests/c/external/gcc_torture/src/20030715-1.c`: failing call is
  `server_type` block 0 inst 4, `callee=ap_check_cmd_context`,
  `result=ptr %t4`. Prepared result is `value_bank=gpr`,
  `source_storage=register`, `source_reg=a0`, `destination_storage=stack_slot`,
  `destination_value_id=9`, `destination_slot=#8`, `dest_stack_offset=56`;
  value home/storage agree: `%t4` is stack slot #8 at stack+56, width 1.
  In scope for pointer stack-result policy. Adjacent bucket:
  arg1 is a scalar `frame_slot_value` source with
  `missing_frame_slot_arg_publication=yes`, which must not be folded into this
  idea.
- `tests/c/external/gcc_torture/src/20011113-1.c`: failing call is `foo`
  block 0 inst 16, `callee=bar`, `result=ptr %t8`; a second same-shape
  in-scope call is `baz` block 0 inst 19, `result=ptr %t17`. Both prepared
  results are GPR call results from `a0` to stack slot stack+112
  (`%t8` value_id 12 slot #29, `%t17` value_id 26 slot #38), width 1, with
  `call_result_stack_to_register` after-call moves. In scope for pointer
  stack-result policy. Adjacent bucket: outgoing byval aggregate argument
  transport (`value_bank=aggregate_address`, stack-copy payload, dest stack
  area 24) is separate aggregate outgoing-stack work.
- `tests/c/external/gcc_torture/src/20041218-1.c`: failing call is `bar`
  block 0 inst 15, `callee=dummy1`, `result=ptr %t5`. Prepared result is
  `value_bank=gpr`, from `a0` to stack slot #29 at stack+104,
  `destination_value_id=28`; value home/storage agree and width is 1.
  In scope. Later pointer aggregate-copy stores and additional stack-slot
  temporaries are adjacent aggregate/local-memory evidence only.
- `tests/c/external/gcc_torture/src/pr20601-1.c`: failing call is `setup1`
  block 0 inst 0, `callee=baz`, `result=ptr %t3`. Prepared result is GPR from
  `a0` to stack slot, `destination_value_id=14`, `destination_slot=#22`,
  `dest_stack_offset=64`; home/storage agree, width 1. In scope. The file also
  contains extensive global/local pointer traffic; that remains local/global
  or memory-access adjacency, not this packet.
- `tests/c/external/gcc_torture/src/pr34176.c`: failing call is `main`
  block 6 inst 1, `callee=foo`, `result=ptr %t12`. Prepared result is GPR from
  `a0` to stack slot #12 at stack+88, `destination_value_id=15`; home/storage
  agree, width 1. In scope. Adjacent negative evidence includes ordinary
  scalar/register result calls and stack-slot preserved values, but the first
  RV64 rejection is the pointer stack-result call.
- `tests/c/external/gcc_torture/src/pr58209.c`: failing call is `bar`
  block 2 inst 1, `callee=foo`, `result=ptr %t7`. Prepared result is GPR from
  `a0` to stack slot #3 at stack+8, `destination_value_id=13`; home/storage
  agree, width 1. In scope. Adjacent negative evidence: `foo` has a similar
  pointer result to register `s2` and `main` has scalar/frame-slot argument
  publication gaps; neither is the pointer stack-result home bucket.

Current shared blocker: prepared call plans already carry callsite identity,
result value id, stack slot id, stack offset, GPR source placement, and width,
but RV64 object-route admission rejects pointer-typed stack destinations. In
`src/backend/mir/riscv/codegen/object_emission.cpp`, the stack-slot result
consumer explicitly returns unsupported for `call.result->type == Ptr` before
using the existing stack-home check. The missing policy is explicit pointer
stack-result admission/fail-closed validation, not raw home discovery.

## Suggested Next

Step 2 should inspect the producer/consumer surface around
`build_call_result_plan(...)` in `src/backend/prealloc/call_plans.cpp`,
`PreparedCallResultPlan` / `find_prepared_call_result_late_publication(...)` in
`src/backend/prealloc/calls.hpp`, and the same-module stack-slot result
consumer in `src/backend/mir/riscv/codegen/object_emission.cpp`.

Confirm whether existing carriers are sufficient for pointer result
destination/home authority. If they are sufficient, define the fail-closed
consumer predicate that admits only ordinary same-module pointer results with
source register, GPR bank, destination value id, destination stack slot id,
stack offset, width 1, matching prepared value home, and no aggregate/FPR/
variadic/library/local-global inference.

## Watchouts

- Do not use source-file names, final assembly shape, ABI register names alone,
  or aggregate shape as pointer result-home authority.
- Do not fold scalar-only, scalar frame-slot argument publication, aggregate
  outgoing-stack/byval transport, FPR, variadic, library, runtime,
  local/global, expectation, unsupported marker, allowlist, timeout, or
  accounting work into this idea.
- `20011113-1.c` has both in-scope pointer stack-result rows and an adjacent
  outgoing byval aggregate stack-copy argument bucket.
- `20030715-1.c` and `pr58209.c` expose frame-slot argument publication gaps
  near the pointer result calls; those are adjacent input-argument buckets.
- `pr20601-1.c` has broad local/global pointer memory traffic; use the
  prepared call result facts above for this idea, not the surrounding global
  stores/loads.
- Treat testcase-shaped shortcuts as route drift.

## Proof

Command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed, 347/347 backend tests. Proof log: `test_after.log`.

Temporary evidence was written under `/tmp/c4c627_step1_evidence` and is not
canonical lifecycle state.
