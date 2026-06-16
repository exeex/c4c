Status: Active
Source Idea Path: ideas/open/287_post_286_prepared_boundary_interface_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Execute One Narrow Cleanup or Generate Follow-Ups

# Current Packet

## Just Finished

Completed Step 6 by implementing the selected narrow structured
`alignstack` call-argument metadata cleanup for the AArch64 variadic HFA
carrier-array path.

Implementation:

- Added `aarch64_stack_align_bytes` to `OwnedLirTypedCallArg` and
  `LirCallArg`.
- Kept legacy rendered call-argument text compatible by teaching
  `format_lir_typed_call_arg(const OwnedLirTypedCallArg&)` to render
  `alignstack(N)` when the structured metadata is present.
- Changed the AArch64 variadic HFA carrier producer in
  `src/codegen/lir/hir_to_lir/call/args.cpp` to store the carrier type as
  plain `[N x T]`, attach `LirTypeRef("[N x T]")`, and publish
  `aarch64_stack_align_bytes` instead of embedding `alignstack(...)` in the
  structured type text.
- Extended `BirFunctionLowerer::lower_call_inst` metadata application so
  structured call arguments override BIR `CallArgAbiInfo::align_bytes` from
  `aarch64_stack_align_bytes`.
- Threaded the original call-argument index into
  `append_aarch64_variadic_hfa_carrier_arg_lanes` so the expanded HFA carrier
  lanes receive the structured stack-alignment fact without parsing
  `alignstack(...)` from type text.
- Updated the focused backend note test so its structured call argument uses
  stripped type text plus `aarch64_stack_align_bytes`, while its rendered
  `args_str` / legacy `arg_type_refs` still exercise `alignstack(8)`
  compatibility.

## Suggested Next

Supervisor should review and commit this coherent Step 6 code slice plus this
`todo.md` update. After that, use plan-owner review/closure routing to decide
whether the runbook is complete or should emit the Step 4 / Step 5 follow-up
ideas.

## Watchouts

`strip_call_arg_abi_type_suffix` is still required for legacy/no-ref rendered
compatibility and byval parsing outside this narrow structured carrier path.
Do not remove it until every relevant caller has structured ABI metadata.

The opaque pointer byte-offset provenance follow-up from Step 5 remains
separate; do not fold it into this call-argument cleanup.

No HFA helper extraction was attempted in this packet. The target ABI lane
expansion boundary from Step 4 remains a follow-up candidate, not part of this
slice.

## Proof

Ran exactly:

```bash
cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_cli_dump_bir_00204_stdarg_movi_zext_immediate_fold|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$' >> test_after.log 2>&1
```

Result: passed. `test_after.log` contains the build proof and 3/3 focused tests
passing.
