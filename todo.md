Status: Active
Source Idea Path: ideas/open/613_abi_call_result_stack_frame_lowering.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Residual Split Or Close-Readiness Classification

# Current Packet

## Just Finished

Completed the Step 4 residual refresh after the scalar GPR same-module
call/result consumer slice.

Focused current-build probes were written under
`build/agent_state/613_step4_residual_refresh.tsv` and
`build/agent_state/613_step4_prepared_dumps/`. The refreshed probe set confirms
the accepted Step 4 movement:

- `src/20001017-2.c` and `src/20010118-1.c` compile through `--codegen obj`.
- `src/20001101.c` moved to `unsupported_terminator_fragment`.
- `src/20040625-1.c` moved to `unsupported_move_bundle_target_shape`.
- Additional stale `unsupported_call_abi` rows now compiling through object
  include `src/20050613-1.c`, `src/980701-1.c`, `src/pr20466-1.c`, and
  `src/pr77767.c`.

Remaining refreshed ABI-adjacent rows in the focused set were classified as:

- `39` `unsupported_call_abi` rows.
- `13` `unsupported_stack_frame` rows.
- `4` `unsupported_move_bundle_target_shape` rows.
- `1` `unsupported_terminator_fragment` row.
- `1` `malformed_prepared_join_transfer_carrier` row.

No remaining same-authority Step 4 ABI consumer packet with complete prepared
facts and meaningful breadth was found. Representative residual owners:

- Aggregate/byval and outgoing stack transport: `src/20000808-1.c`,
  `931004-*`, `src/931031-1.c`, `src/950607-2.c`, and `src/pr69447.c`.
- Missing prepared producer/publication authority:
  `src/20020529-1.c`, `src/20010129-1.c`, `src/20020406-1.c`, and
  frame-slot argument publication rows such as `src/pr58209.c`.
- Pointer stack-result policy: `src/20030715-1.c`, `src/20011113-1.c`,
  `src/20041218-1.c`, `src/pr20601-1.c`, `src/pr34176.c`, and
  `src/pr58209.c`.
- FPR/frame policy: `src/980605-1.c`, `src/ieee/compare-fp-2.c`,
  `src/ieee/unsafe-fp-assoc.c`, `src/pr39501.c`, and the refreshed
  `unsupported_stack_frame` rows.
- Return destination-home authority remains separated under
  `return_stack_to_register` move-bundle ownership for `src/20001130-2.c` and
  `src/20080719-1.c`.

## Suggested Next

Advance to Step 5 residual split/close-readiness classification for idea 613.
Use the current residual refresh to decide whether idea 613 is close-ready or
needs additional durable splits for pointer stack-result policy, FPR/frame
policy, aggregate outgoing stack transport, return destination-home authority,
and remaining prepared producer/publication gaps.

## Watchouts

- `src/20000808-1.c` remains under idea 624 at `unsupported_call_abi`.
- `src/20020529-1.c` remains under idea 625 at `unsupported_call_abi`.
- `src/20040811-1.c` remains under idea 626 at `unsupported_stack_frame`.
- `src/20001130-2.c` and `src/20080719-1.c` remain return destination-home
  authority gaps under `return_stack_to_register` move-bundle ownership.
- `src/20021219-1.c` remains downstream
  `malformed_prepared_join_transfer_carrier`; `src/pr77767.c` still compiles.
- Do not infer pointer stack-result policy from `src/20030715-1.c`; it needs a
  separate decision because the existing pointer stack-result guard is covered
  by a fail-closed object-emission test.
- Do not treat FPR argument/result rows, dynamic/static frame policy rows,
  runtime/library/variadic rows, generic move-bundle rows, local/global
  producer rows, or terminator/instruction-fragment owners as Step 4 ABI
  consumer progress.

## Proof

Supervisor-delegated proof ran exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed, `346/346` backend tests. `test_after.log` reports
`100% tests passed, 0 tests failed out of 346`.
