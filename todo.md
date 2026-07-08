Status: Active
Source Idea Path: ideas/open/613_abi_call_result_stack_frame_lowering.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Residual Split Or Close-Readiness Classification

# Current Packet

## Just Finished

Completed Step 5 residual split/close-readiness classification for idea 613.

Current residual evidence comes from the refreshed probe table at
`build/agent_state/613_step4_residual_refresh.tsv` and prepared dumps under
`build/agent_state/613_step4_prepared_dumps/`. The focused residual set now
contains:

- `39` `unsupported_call_abi` rows.
- `13` `unsupported_stack_frame` rows.
- `4` `unsupported_move_bundle_target_shape` rows.
- `1` `unsupported_terminator_fragment` row.
- `1` `malformed_prepared_join_transfer_carrier` row.
- `6` rows that now compile through `--codegen obj`.
- `1` missing-source row.

No remaining in-scope ABI/RV64 consumer family was found with complete prepared
facts and meaningful breadth. The ordinary same-module scalar GPR call/result
consumer work under idea 613 has either landed, moved rows to downstream
owners, or exposed producer/policy gaps that must not be inferred in RV64.

Residual owner classification:

- Already split producer gaps:
  - idea 624 covers prepared outgoing stack argument destination offsets for
    `src/20000808-1.c` and adjacent byval/outgoing-stack aggregate rows.
  - idea 625 covers prepared stack-slot preservation source publication for
    `src/20020529-1.c`.
  - idea 626 covers prepared dynamic-frame callee-saved GPR save-slot
    placement for rows such as `src/20040811-1.c`, `src/pr43220.c`, and
    `src/vla-dealloc-1.c`.
- Additional durable split recommendations:
  - pointer stack-result call policy/authority, represented by
    `src/20030715-1.c`, `src/20011113-1.c`, `src/20041218-1.c`,
    `src/pr20601-1.c`, `src/pr34176.c`, and `src/pr58209.c`.
  - FPR ABI/frame policy and prepared FPR frame placement, represented by
    `src/980605-1.c`, `src/ieee/compare-fp-2.c`,
    `src/ieee/unsafe-fp-assoc.c`, `src/pr39501.c`, and FPR-heavy
    `unsupported_stack_frame` rows.
  - prepared return destination-home authority for `return_stack_to_register`
    move-bundle rows `src/20001130-2.c` and `src/20080719-1.c`.
  - broader aggregate/outgoing-stack argument transport beyond the current
    idea 624 representative, including `931004-*`, `src/931031-1.c`,
    `src/950607-2.c`, and `src/pr69447.c`, unless the plan owner chooses to
    treat them as covered by the idea 624 outgoing-stack authority route.
- Existing non-613 routes cover or should retain unrelated residual owners:
  downstream move-bundle authority, local/global producers, runtime/library or
  variadic policy, terminator/instruction-fragment stops, and the
  `src/20021219-1.c` prepared join-transfer carrier blocker.

## Suggested Next

Hand off to the plan owner. Recommendation: close or retire idea 613 after the
plan owner records the residual splits above, because the active ABI/RV64
consumer route has no remaining complete-authority implementation packet with
meaningful breadth.

## Watchouts

- Do not treat the remaining `unsupported_call_abi` label as sufficient proof
  that a row still belongs to idea 613; the refreshed prepared dumps show the
  live blockers are missing authority, pointer stack-result policy, FPR/frame
  policy, aggregate/outgoing-stack transport, or unrelated downstream owners.
- Do not infer outgoing stack offsets, preserve sources, callee-saved frame
  placements, return destination homes, or pointer stack-result rules inside
  RV64 object emission.
- If the plan owner creates new ideas, keep them producer/policy scoped rather
  than named-case scoped. The representative rows are evidence, not the
  implementation contract.

## Proof

Supervisor-delegated proof ran exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed, `346/346` backend tests. Proof log path: `test_after.log`.
