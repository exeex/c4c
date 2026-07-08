Status: Active
Source Idea Path: ideas/open/613_abi_call_result_stack_frame_lowering.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement First Ordinary ABI Consumer

# Current Packet

## Just Finished

Completed Step 2, "Implement First Ordinary ABI Consumer", for the ordinary
same-module prepared call-argument frame-address consumer. The RV64 object
route now accepts identical duplicate prepared frame-slot address
materialization facts for a call argument, while preserving rejection for
conflicting materialization facts. This lets a prepared call plan consume two
ABI argument bindings that intentionally reuse the same published frame
address, instead of treating the duplicate publication as ambiguous.

Representative behavior:

- `src/20000603-1.c` moved past `unsupported_call_abi` at `main`
  `block_index=0`, `instruction_index=1`, `callee=f`, `args=2`,
  `planned_args=2`, `result=double %t4`. Its first stop is now
  `unsupported_terminator_fragment`, so the same-module call/result consumer
  no longer owns that row.
- `src/20000808-1.c` remains at `unsupported_call_abi` for the broader
  byval/outgoing-stack aggregate argument shape. Its prepared plan mixes
  aggregate-address stack-copy arguments with stack-slot address arguments and
  should be a separate same-module call packet, not folded into the duplicate
  address-materialization fix.
- Focused RV64 object-emission coverage now accepts identical duplicate
  frame-slot address materialization facts for prepared call arguments and
  keeps conflicting duplicate offsets rejected.

Guard probes stayed on their prior owners:

- `src/20020314-1.c`, `src/strcpy-2.c`: `unsupported_stack_frame`.
- `src/20001130-2.c`, `src/20080719-1.c`: before-return
  `return_stack_to_register` under `unsupported_move_bundle_target_shape`.
- `src/920411-1.c`: generic move-bundle materialization, not return ABI.
- `src/20000722-1.c`: `unsupported_local_memory_access`.
- `src/20030828-1.c`: `unsupported_global_data`.
- `src/va-arg-2.c`, `src/va-arg-12.c`, `src/va-arg-24.c`: remained outside
  the ordinary same-module packet.

## Suggested Next

Reviewer split verdict recorded in
`review/613_step2_byval_outgoing_stack_slice_review.md`: the
`src/20000808-1.c` byval/outgoing-stack aggregate argument shape is blocked by
missing prepared destination stack offsets, which is producer authority outside
idea 613.

The producer gap is split to
`ideas/open/624_prepared_outgoing_stack_argument_destination_offsets.md`.

Continue idea 613 under Step 2 only with ordinary same-module call/result
consumer rows whose prepared call/result facts already include the needed
stack/register boundary authority. Before the next code packet, run a focused
residual scan to select either:

- another Step 2 ordinary call/result consumer family with complete prepared
  facts, or
- a no-breadth handoff to Step 3 stack-frame/return handling if no such Step 2
  family remains.

## Watchouts

- Keep variadic, library, runtime, missing prepared-authority, local/global
  producer, expectation, unsupported-marker, allowlist, timeout, and accounting
  work out of this idea.
- Do not infer prepared call/return/frame facts from final assembly or
  testcase names.
- `src/20000603-1.c` now stops at terminator lowering; do not claim it as fully
  backend-supported from this packet.
- `src/20000808-1.c` is still ordinary same-module call ABI, but its
  aggregate-address stack-copy plus outgoing stack-slot argument shape lacks
  prepared destination stack offsets. Treat that as split producer work under
  idea 624, not as immediate 613 consumer scope.

## Proof

Ran exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed, `346/346` backend tests; proof log is `test_after.log`.
