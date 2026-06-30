Status: Active
Source Idea Path: ideas/open/450_select_result_branch_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Select-Result Branch Evidence

# Current Packet

## Just Finished

Completed Step 1: audited select-result branch and move-bundle materialization
evidence for idea 450. Supporting artifact:
`build/agent_state/450_step1_select_result_audit/classification.md`.

Bucket summary:

| Bucket | Representative rows | Classification | First missing fact |
| --- | --- | --- | --- |
| Accepted candidate for first packet | `20010329-1 main.logic.end.14` `%t22`, `main.logic.end.27` `%t36`, `main.logic.end.41` `%t50` | Each row has `root_is_select=yes`, `source_producer=select_materialization`, `ne i32 <select>, 0` prepared branch condition, GPR-compatible select-result/condition homes, and available register-destination `block_entry_publication` move-bundle facts. Current object result is `unsupported_move_bundle_target_shape`. | Explicit RV64 consumer/materialization contract for register-home select-result block-entry publications tied to branch conditions. |
| Candidate fact, not first owner | `20000622-1 foo.logic.end.7` `%t13`, `foo.logic.end.18` `%t24` | Select-result branch facts and register block-entry publications exist, but the current object route fails first at `unsupported_instruction_fragment`. | Instruction-side lowering must move first for this representative before it can prove select-result branch consumption. |
| Rejected from first packet | `930930-1 f.logic.end.14` `%t22` inherited from 449 evidence | Select-chain evidence exists, but the selected value has stack-slot home and `block_entry_publication status=unsupported_destination_storage`; the object route also still has earlier unsupported instruction and pointer-value/provenance owners. | Stack-home branch operand / move-bundle materialization belongs to idea 451; pointer-value/provenance remains separate. |

## Suggested Next

Step 2 should define the select-result publication contract for the first
bounded packet: register-home select-result values produced by explicit
`select_materialization`, consumed by prepared `Eq`/`Ne` against integer zero
branch conditions, and backed by available register-destination
`block_entry_publication` / out-of-SSA move-bundle facts. The first future
implementation packet should consume that contract for the `20010329-1`
shape without inferring from raw selects.

Future implementation proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Keep this plan limited to select-result branch publication and move-bundle
  target materialization.
- Do not reopen pointer `eq/ne` or unsigned relational branch publication;
  those routes are closed by ideas 441 and 449.
- Do not fold stack-home branch operand materialization into this plan; that
  belongs to `ideas/open/451_stack_home_branch_operand_materialization.md`.
- Keep `20000622-1` out of the first implementation packet while its current
  first object-route owner is `unsupported_instruction_fragment`.
- Keep `930930-1` stack-slot select-result publication out of this packet;
  its current prepared rows are not register-destination consumer facts.
- Do not infer select-result publication from raw select shape,
  `root_is_select=yes`, raw `ne i32 <select>, 0`, block order, filenames,
  function names, or one dump layout.
- Keep missing or incoherent select-result publication authority fail-closed.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 1 classification-only validation:

```sh
git diff --check
```

Result: passed.
