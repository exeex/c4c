Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Local-Frame Publication Authority Leak

# Current Packet

## Just Finished

Step 1 selected one remaining local-frame publication authority leak in
`src/backend/mir/aarch64/codegen/calls_argument_sources.cpp`: the
`call_argument_allows_local_frame_address_publication` helper path at lines
475-509.

- Retained metadata read: `call_argument_is_pointer` reads
  `CallInst::arg_types[arg_index]` and falls back to `call->args[arg_index].type`
  to decide pointer eligibility.
- Retained metadata read: `call_argument_is_byval_copy` reads
  `CallInst::arg_abi[arg_index].byval_copy` to reject byval copies.
- Retained call decision: `call_argument_allows_local_frame_address_publication`
  also reads `call->callee.rfind("llvm.", 0)` to reject LLVM intrinsics.
- Prepared replacement route: use
  `PreparedCallArgumentPlan::allows_local_aggregate_address_publication` instead
  of rederiving this decision from `CallInst`; it is populated in
  `src/backend/prealloc/call_plans.cpp` by
  `call_argument_allows_local_aggregate_address_publication`, which performs the
  same callee, `arg_abi.byval_copy`, `arg_types` pointer, and argument-type
  fallback decision before publishing each argument plan.
- Evidence commands used:
  `rg -n "arg_abi|arg_types|allows_local|local|aggregate|address|publication|byval|sret|Ptr" src/backend/mir/aarch64/codegen/calls_argument_sources.cpp`,
  `nl -ba src/backend/mir/aarch64/codegen/calls_argument_sources.cpp | sed -n '300,740p'`, and
  `nl -ba src/backend/prealloc/call_plans.cpp | sed -n '880,1010p;1280,1475p;1550,1665p'`.

## Suggested Next

Execute Step 2 by replacing the selected helper path in
`calls_argument_sources.cpp` so `make_frame_slot_call_argument_address_source`
and `make_local_frame_address_call_argument_source` gate local-frame address
publication on `argument.allows_local_aggregate_address_publication` rather than
calling back into `CallInst` for `arg_types`, `arg_abi`, or callee spelling.

## Watchouts

- Do not work on `ideas/open/03_dispatch_responsibility_reduction.md`.
- Do not weaken tests or mark a nearby publication case unsupported to claim
  progress.
- The prepared fact name says `aggregate`, but its published decision is the
  local-frame publication eligibility needed here. Keep the next packet scoped
  to consuming that fact; do not expand into unrelated argument source lowering.
- After the replacement, no retained `CallInst::arg_abi` or `CallInst::arg_types`
  reads should remain in this local-frame publication helper path.

## Proof

Selection-only lifecycle scratchpad update. No build or test command was run.
Next Step 2 proof command:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`.
