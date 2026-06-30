Status: Active
Source Idea Path: ideas/open/452_select_edge_source_producer_rematerialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Select-Edge Rematerialization Evidence

# Current Packet

## Just Finished

Completed Step 1: audited select-edge source-producer rematerialization
evidence for idea 452. Supporting artifact:
`build/agent_state/452_step1_select_edge_rematerialization_audit/audit.md`.

Bucket summary:

| Bucket | Representative edges | Classification | First missing authority |
| --- | --- | --- | --- |
| First blocker | `logic.rhs.end.13 -> logic.end.14`, `%t18 -> %t22` | `%t18` is a successor/join-block compare over `%t15` and `%t17`; `%t15` has register home `s1`, `%t17 = inttoptr i32 %t16 to ptr`, `%t16` is rematerializable immediate, and `%t17` has stack-slot pointer home `slot_id=2 offset=16`. The select publication and block-entry destination facts for `%t22` are coherent, but `%t18` is not copy-available on the predecessor edge. | Durable prepared rematerialization dependency authority for the compare/cast chain, plus a policy for stack-slot pointer dependency `%t17`. |
| Register/immediate compare-source candidates | `logic.rhs.end.26 -> logic.end.27`, `%t32 -> %t36`; `logic.rhs.end.40 -> logic.end.41`, `%t46 -> %t50` | `%t32` and `%t46` are also successor/join-block compare results, but their dependency operands currently have register-compatible homes. They are plausible narrower rematerialization candidates after the first-edge contract is defined. | Edge rematerialization dependency record and placement/availability proof; not first because `%t22` blocks earlier. |
| Already materializable immediate sources | `0 -> %t22`, `0 -> %t36`, `0 -> %t50` | Immediate sources are encoded directly in prepared move payloads and are edge-available. | None for this source class. |

## Suggested Next

Step 2 should define the select-edge source-producer rematerialization
contract. It should separate a narrow register/immediate dependency-chain
candidate (`%t32/%t46`) from the `%t18` first blocker that includes stack-slot
pointer dependency `%t17`, and decide whether that stack-slot dependency is
rejected, routed to idea 451, or accepted only under an additional authority
contract.

Future implementation proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Do not copy `%t18` on the predecessor edge unless edge availability is
  proven.
- Do not infer source-producer dependencies from raw BIR shape, block order,
  filenames, function names, or one prepared dump layout.
- Keep stack-home branch operand materialization routed to
  `ideas/open/451_stack_home_branch_operand_materialization.md` unless Step 2
  explicitly accepts a narrow overlap as part of edge rematerialization.
- Keep pointer-value provenance and generic instruction-side lowering out of
  this plan.
- Keep `20000622-1` out of the first packet while its first owner is
  instruction-side lowering.
- Do not treat a successor-defined compare result's register home as
  predecessor-edge availability.
- Do not let the later `%t32/%t46` register-compatible candidates hide the
  first `%t18/%t17` stack-slot dependency decision.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 1 classification-only validation:

```sh
git diff --check
```

Result: passed.
