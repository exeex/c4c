Status: Active
Source Idea Path: ideas/open/453_stack_slot_pointer_select_edge_dependency_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Stack-Slot Pointer Dependency Evidence

# Current Packet

## Just Finished

Completed Step 1 for idea 453. The audit artifacts are under
`build/agent_state/453_step1_stack_slot_pointer_dependency_audit/`.

Bucket table:

| Bucket | Edge | Incoming value | Compare producer | `%t17` cast producer | Stack-slot home | Slot metadata | Source immediate | Edge placement | Target register | Current prepared authority | First missing fact |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| First in-scope blocker | `logic.rhs.end.13 -> logic.end.14` | `%t18 -> %t22` | `%t18 = bir.ule ptr %t15, %t17`; `%t15` home `s1`. | `%t17 = bir.inttoptr i32 %t16 to ptr`. | `%t17` stack slot `slot_id=2 offset=16`. | `object #2` is `type=ptr size=8 align=8 address_exposed=no requires_home_slot=no permanent_home_slot=no`. | `%t16` rematerializable immediate `-2147483643`. | Predecessor terminator in `logic.rhs.end.13`; `%t18` is successor-defined and not copy-available. | `%t22` register `t0`. | `select_materialization` join transfer, available register `block_entry_publication`, and `select_chain root_is_select=yes`. | Explicit prepared authority for stack-slot pointer dependency materialization: load `%t17`, rematerialize `%t17` from `%t16`, or fail closed with slot identity, width, freshness/clobber safety, placement, and target compatibility. |
| Already supported edge source | `logic.skip.12 -> logic.end.14` | `0 -> %t22` | Immediate move payload. | N/A | N/A | N/A | Immediate `0`. | Predecessor terminator in `logic.skip.12`. | `%t22` register `t0`. | Prepared move payload plus available register block-entry publication. | None; not the blocker. |
| Accepted dependency operand | Dependency operand of `%t18` | `%t15` | `%t15 = bir.load_local ptr %lv.x`. | N/A | N/A | Local access is `frame_slot=#0 offset=0 size=8 align=8 base_plus_offset=yes range_verdict=proven_in_bounds`, with `layout_authority=unknown`. | N/A | Register operand is available to the compare rematerialization path. | `%t15` register `s1`. | GPR-compatible home. | No first missing fact for this idea. |
| Closed sibling class | `%t32 -> %t36`, `%t46 -> %t50` | Successor compare sources | Register/immediate dependency compares validated by idea 452. | N/A | N/A | N/A | Existing register/immediate facts. | Predecessor-edge rematerialization. | Register destinations. | Covered by existing idea 452 route and tests. | None in idea 453. |

First missing fact: a prepared stack-slot pointer dependency materialization
authority tied to the select-edge move. Current facts do not authorize copying
`%t18`, loading `%t17` from stack, or bypassing the stack slot by
rematerializing `%t17` from `%t16`/`inttoptr`.

## Suggested Next

Step 2: `Define Stack-Slot Pointer Dependency Contract`.

Define the accepted/rejected authority for materializing `%t17` as a
select-edge compare dependency: stack-slot load authority, cast
rematerialization authority from `%t16`, required producer/home metadata, or a
split if the first owner is general stack-home branch materialization.

Future implementation proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Do not copy `%t18` from its successor/join-block register home on the
  predecessor edge.
- Do not load or rematerialize `%t17` from a stack slot without explicit
  prepared authority for slot identity, width, freshness, clobber safety, edge
  placement, and target compatibility.
- Do not infer dependency materialization authority from raw `inttoptr`,
  stack-slot spelling, block order, filenames, function names, or one prepared
  dump layout.
- Keep general stack-home branch consumer work routed to
  `ideas/open/451_stack_home_branch_operand_materialization.md`.
- Keep pointer-value provenance and generic instruction-side lowering out of
  this plan.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 1 classification proof:

```sh
git diff --check
```

Result: passed.
