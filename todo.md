Status: Active
Source Idea Path: ideas/open/329_aarch64_variadic_aggregate_va_arg_call_setup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Validate Representative And Classify Residuals

# Current Packet

## Just Finished

Step 4 validated idea 329's post-`va_arg` ordinary-call operand publication on
the focused `00204.c` representative and classified the remaining runtime
mismatch.

Generated `myprintf` now publishes the `%7s` branch call operands before
`bl printf`: `build/c_testsuite_aarch64_backend/src/00204.c.s` lines
6127-6130 load `.str31` (`"%.7s"`) into `x0`, set `x1` to `sp + 8`, then
branch. The `%9s` branch has the same repaired call-publication shape at
lines 6174-6177: `.str33` (`"%.9s"`) is loaded into `x0`, `x1` is set to
`sp + 15`, then `bl printf` runs. The old first bad fact where libc `printf`
could be reached with `x0 == x1` instead of the format-string pointer is gone.

The residual `c_testsuite_aarch64_backend_src_00204_c` failure is not still a
`.str31`/`.str33` fixed-operand publication fault. Fresh generated-code
evidence localizes the next owner to non-HFA aggregate `va_arg` value
materialization: the `%7s` and `%9s` paths compute/select a `va_arg` source
through `x13`, but the branch-local code from lines 6027-6180 never copies the
selected aggregate bytes into the stack buffers later passed as `x1` (`sp + 8`
and `sp + 15`). The runtime output confirms this: after `stdarg:`, the first
string lines are corrupted (`TX\`d`, `lptx|`, then binary garbage) instead of
the expected `ABCDEFGHI` / `lmnopqr` non-HFA aggregate text.

Classification: distinct adjacent initiative, not further idea 329
post-consumption call setup and not yet parked idea 326 HFA/floating residual.
The call boundary is now publishing the format and aggregate-buffer operands;
the exposed blocker is that the aggregate buffer being published is not
materialized from the selected non-HFA `va_arg` source before the following
ordinary call observes it.

## Suggested Next

Smallest next packet: create/activate a distinct adjacent non-HFA aggregate
`va_arg` materialization initiative for generated `myprintf`. Localize the
selected `va_arg` source for the `%7s` and `%9s` branches, then repair the
general AArch64 path that copies non-HFA aggregate bytes from the register-save
area or overflow area into the temporary aggregate object before publishing its
member address to `x1` for the following call.

## Watchouts

The representative still has broad downstream output corruption, including
HFA/floating, return-value, and MOVI mismatches in `test_after.log`. Do not
pull those into idea 329. The first freshly exposed post-call-setup bad fact is
earlier: the non-HFA `stdarg` string output is wrong even though the `printf`
call operands are in `x0`/`x1`. Park idea 326 until the non-HFA aggregate
`va_arg` materialization owner either clears the string residual or advances
the representative to a clean HFA/floating first bad fact.

## Proof

Ran the delegated proof command exactly:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_cli_dump_bir_00204_stdarg_semantic_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication|backend_cli_dump_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_function_filters_00204|backend_cli_dump_prepared_bir_focus_block_entry_00204|backend_prepare_liveness|backend_aarch64_machine_printer|backend_aarch64_instruction_dispatch|backend_aarch64_target_instruction_records|c_testsuite_aarch64_backend_src_00204_c)$'`.

Result: build completed with `ninja: no work to do`; 11 of 12 tests passed.
`backend_lir_to_bir_notes`, the `00204.c` semantic/prepared/focus dump checks,
`backend_prepare_liveness`, `backend_aarch64_machine_printer`,
`backend_aarch64_instruction_dispatch`, and
`backend_aarch64_target_instruction_records` all passed.
`c_testsuite_aarch64_backend_src_00204_c` failed with
`[RUNTIME_MISMATCH]`, now classified above as a distinct adjacent non-HFA
aggregate `va_arg` materialization residual. Proof log: `test_after.log`.
