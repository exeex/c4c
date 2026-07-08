Status: Active
Source Idea Path: ideas/open/613_abi_call_result_stack_frame_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh ABI Consumer Ownership

# Current Packet

## Just Finished

Completed Step 1, "Refresh ABI Consumer Ownership", as a diagnostic packet.
Current scan evidence under `build/rv64_gcc_c_torture_backend` splits the
idea-613 surface into:

- Ordinary same-module ABI/RV64 consumer: `52` current
  `unsupported_call_abi` rows. Representatives:
  - `src/20000603-1.c`: `function=main`, `callee=f`, `args=2`,
    `planned_args=2`, `result=double %t4`; semantic BIR has
    `bir.call double f(...)`, and prepared BIR publishes a same-module call
    plan with two GPR arguments, an FPR result, call clobbers, and frame facts.
  - `src/20000808-1.c`: `function=foo`, `callee=f`, `args=6`,
    `planned_args=6`, `result=none`; prepared BIR publishes a same-module call
    plan with aggregate-address and frame-slot argument sources, stack-copy
    chunks, freshness authority, and clobbers.
- Supported stack-frame layout consumer: `12` current
  `unsupported_stack_frame` rows. Representatives:
  - `src/20020314-1.c`: prepared frame plan publishes `g` with
    `frame_size=56`, `has_dynamic_stack=yes`, `fixed_slots_use_fp=yes`, and
    `main` with `frame_size=48` plus callee-saved GPR/FPR save facts.
  - `src/strcpy-2.c`: remains a stack-frame guard row; do not fold library or
    string-policy work into ordinary call lowering.
- Return move-bundle target consumer with prepared return facts: `2` current
  `return_stack_to_register` rows inside the broader
  `unsupported_move_bundle_target_shape` bucket.
  - `src/20001130-2.c`: before-return move to `function_return_abi`,
    destination register `a0`, source home `stack_slot`, `source_type=ptr`,
    `destination_type=ptr`.
  - `src/20080719-1.c`: same before-return stack-slot pointer to `a0` shape,
    with two return blocks in prepared BIR.
- Out-of-scope or downstream guard rows:
  - `src/920411-1.c`: generic move-bundle materialization with
    `consumer_register_to_stack` plus `consumer_stack_to_stack`, not return
    ABI.
  - `src/20000722-1.c`: `unsupported_local_memory_access`, producer/local
    memory owner.
  - `src/20030828-1.c`: `unsupported_global_data`, producer/global owner.
  - `src/20011024-1.c`: runtime/intrinsic `memcpy runtime family`, not ABI
    consumer progress.
  - `src/va-arg-2.c`, `src/va-arg-12.c`, `src/va-arg-24.c`: variadic rows with
    local-memory or variadic policy ownership, not Step 2 candidates.
  - `src/pr38533.c`, `src/pr83298.c`, `src/20080506-2.c`: runtime abort or
    segfault representatives from the post-contract runtime notes; keep them
    separate from object-route ABI lowering.

## Suggested Next

Step 2 should implement the first ordinary same-module ABI/RV64 call/result
consumer packet, starting from the `unsupported_call_abi` rejection. Use
`src/20000603-1.c` and `src/20000808-1.c` as positive breadth rows because both
have complete prepared call-plan facts but cover different shapes: scalar/FPR
result arguments and aggregate/frame-slot argument transport. Keep stack-frame
rows and return stack-to-register rows as later idea-613 packets, not part of
this first implementation packet.

## Watchouts

- Keep variadic, library, runtime, missing prepared-authority, local/global
  producer, expectation, unsupported-marker, allowlist, timeout, and accounting
  work out of this idea.
- Do not infer prepared call/return/frame facts from final assembly or
  testcase names.
- Do not treat all `generic_move_bundle_materialization_failed` rows as return
  ABI rows; only the two `destination_kind=function_return_abi` plus
  `reason=return_stack_to_register` rows are in the return subfamily.
- Ordinary call/result lowering must require explicit prepared call plans,
  argument/result locations, widths, call wrapper kind, and clobber facts.

## Proof

Ran exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed, `346/346` backend tests; proof log is `test_after.log`.
