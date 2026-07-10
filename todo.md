Status: Active
Source Idea Path: ideas/open/677_rv64_call_arg_local_frame_address_object_materialization.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reconfirm The Object/Text Divergence

# Current Packet

## Just Finished

Lifecycle switched from closed idea 676 to active idea 677.

## Suggested Next

Delegate Step 1 to an executor: reconfirm the current object/text divergence
for `backend_cli_riscv64_call_arg_local_frame_address_materialization`, read
the prior source-selection evidence, and identify the first object-route
boundary that consumes `LocalFrameAddressMaterialization` differently from the
text route.

## Watchouts

- Do not change the text-route contract unless fresh evidence proves it is
  wrong.
- Do not repair or reclassify the pointer/global-local publication row; idea
  676 is closed.
- Do not accept `test_baseline.new.log`; row 159 is still unresolved.
- Do not edit unsupported markers, allowlists, timeouts, runtime policy, or
  baseline accounting.
- Do not key implementation to the focus test name, filename, or expected byte
  string.

## Proof

No executor proof has run for 677 yet. Close-gate backend proof for 676 showed
the 676 live-load row green and only known residual backend failures at
`backend_cli_riscv64_call_arg_local_frame_address_materialization` and
`backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`.
