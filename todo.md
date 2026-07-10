Status: Active
Source Idea Path: ideas/open/677_rv64_call_arg_local_frame_address_object_materialization.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reconfirm The Object/Text Divergence

# Current Packet

## Just Finished

Step 1: Reconfirm The Object/Text Divergence. Fresh focused proof reproduced
row 159's object-byte failure, captured fresh text/object artifacts under
`build/agent_state/677_step1_object_text_divergence/`, and identified the first
object-route boundary as `fragment_for_prepared_call` in
`src/backend/mir/riscv/codegen/object_emission.cpp`: its
`LocalFrameAddressMaterialization` branch passes the prepared source register
(`s1`) to the local-frame-address helper and then copies to ABI `a0`, while the
text route emits directly to the ABI destination.

## Suggested Next

Delegate Step 2 to an executor: repair or prove the object contract at the
`fragment_for_prepared_call` local-frame-address branch by making the decision
against the `LocalFrameAddressMaterialization` semantic contract, not against
the focused test name or expected-byte string.

## Watchouts

- Do not change the text-route contract unless fresh evidence proves it is
  wrong.
- `append_rv64_prepared_local_frame_address_call_argument_source` already
  encodes direct `addi rd, sp, offset` for the register it is given; the
  divergence is the object caller's current choice of `publication_register =
  source.value_or(*destination)`.
- Do not repair or reclassify the pointer/global-local publication row; idea
  676 is closed.
- Do not accept `test_baseline.new.log`; row 159 is still unresolved.
- Do not edit unsupported markers, allowlists, timeouts, runtime policy, or
  baseline accounting.
- Do not key implementation to the focus test name, filename, or expected byte
  string.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R
'backend_cli_riscv64_call_arg_local_frame_address_materialization'`.
Build succeeded; focused CTest failed as expected with
`[BACKEND_OBJ_MISSING_BYTES]` for missing `13050100`. Proof log:
`test_after.log`.
