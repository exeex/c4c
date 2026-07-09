Status: Active
Source Idea Path: ideas/open/627_pointer_stack_result_call_policy.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Admit Only Explicit Pointer Stack-Result Authority In RV64

# Current Packet

## Just Finished

Completed Step 4 by admitting explicit pointer stack-result authority in the
RV64 same-module call consumer.

Changed files:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`

Behavior:

- Removed the pointer-type-only rejection from the stack-slot call-result
  branch.
- Pointer stack results now remain fail-closed unless the prepared result has
  a GPR source register, one-register source/destination width, explicit
  `source_register_placement=gpr:call_result#.../w1`, destination stack slot
  id/offset, and a matching prepared stack-slot value home.
- Pointer stack-result admission now also requires the BIR result value type
  and call return type to both be `Ptr`; mismatched pointer result/return type
  authority fails closed.
- Pointer stack-result stores use RV64 pointer width, 8 bytes.
- Existing scalar stack-result behavior is unchanged.
- Focused backend coverage now proves a positive ordinary same-module pointer
  result from `a0` into a stack slot with an `sd` store, plus missing placement,
  wrong placement pool, mismatched destination-home, and mismatched result/
  return-type rejection cases.

## Suggested Next

Step 5 should refresh the representative idea-627 rows through the RV64 object
route and classify the remaining owner bucket. Confirm whether the original
pointer stack-result rejection is gone for the in-scope ordinary same-module
rows, then separate any remaining byval/outgoing aggregate, scalar argument,
register-result, variadic/library, or local/global pointer-memory failures as
out-of-scope follow-up work.

## Watchouts

- Pointer stack-result admission intentionally requires explicit
  `CallResult` GPR placement; a bare ABI register name is not enough for
  pointer stack results.
- Keep adjacent buckets out of this idea: scalar frame-slot argument
  publication, byval/outgoing aggregate stack transport, FPR, variadic,
  library/runtime, local/global pointer memory traffic, expectation changes,
  unsupported markers, allowlists, timeout, or accounting work.
- `20011113-1.c` still contains an adjacent outgoing aggregate/byval argument
  stack-copy bucket; Step 5 should not fold that into this idea.

## Proof

Command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed, 347/347 backend tests. Proof log: `test_after.log`.
