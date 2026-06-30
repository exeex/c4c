Status: Active
Source Idea Path: ideas/open/454_edge_dependency_operand_materialization_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Dependency-Operand Metadata Evidence

# Current Packet

## Just Finished

Completed Step 1 for idea 454. The audit artifact is
`build/agent_state/454_step1_dependency_operand_metadata_audit/audit.md`.

Bucket table:

| Bucket | Operand / edge role | Value home | Object metadata | Cast/source identity | Edge placement | Freshness/clobber evidence | Current prepared facts | First missing producer authority |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| First blocker | `%t17`, RHS dependency of `%t18 = bir.ule ptr %t15, %t17` for edge `%t18 -> %t22` | `value_id=9 kind=stack_slot slot_id=2 offset=16`; storage `frame_slot bank=gpr`. | `object #2 func=main name=%t17 source_kind=regalloc.spill_slot type=ptr size=8 align=8 address_exposed=no requires_home_slot=no permanent_home_slot=no`. | `%t17 = bir.inttoptr i32 %t16 to ptr`; `%t16` is rematerializable immediate `-2147483643`. | Predecessor terminator in `logic.rhs.end.13`; `%t18` is successor-defined in `logic.end.14`; destination `%t22` is register `t0`. | No explicit freshness or clobber-safety fact for reading slot #2 at the edge; later call clobber records do not prove this edge-safe. | Edge publication, select chain, `%t17` home/object, and `%t16` immediate facts exist. | Dependency-operand materialization authority with policy `load_from_stack_slot`, `rematerialize_cast_from_source`, or fail-closed, plus slot/object linkage and freshness/clobber proof. |
| Usable adjacent operand | `%t15`, LHS dependency of `%t18` | `value_id=7 kind=register reg=s1`. | Not stack-object backed as the dependency operand; local load access is range-proven. | `%t15 = bir.load_local ptr %lv.x`. | Register operand available before the edge branch. | Register home is already target-consumable. | Accepted by existing register/immediate rematerialization path. | None for idea 454. |
| Possible cast source | `%t16`, source of `%t17` | `value_id=8 kind=rematerializable_immediate imm_i32=-2147483643`. | N/A | `%t16 = bir.add i32 2147483647, 6`; folded immediate in prepared home. | Could feed a cast-rematerialization policy if one is explicit. | Immediate itself has no slot clobber issue. | Immediate home exists. | `rematerialize_cast_from_source` policy plus cast dependency record. |
| Edge source result | `%t18` | `value_id=10 kind=register reg=t0`. | N/A | `%t18 = bir.ule ptr %t15, %t17`. | Defined in successor/join block, not predecessor-edge copy-available. | No edge-availability proof for copying `%t18`. | Select materialization identifies the edge source. | Must remain fail-closed except through dependency-operand rematerialization. |
| Destination | `%t22` | `value_id=11 kind=register reg=t0`. | N/A | `%t22 = bir.select ... i32 %t18, 0`. | Available block-entry destination for both incoming edges. | Destination register placement exists. | `block_entry_publication ... destination_storage=register reg=t0`. | None. |

First missing producer authority: shared dependency-operand materialization
policy representation. The first code packet should not pick only stack-load
or only cast-rematerialization; it needs a common record that can say
`load_from_stack_slot`, `rematerialize_cast_from_source`, or fail-closed with
exact reasons.

## Suggested Next

Step 2: `Define Dependency-Operand Authority Contract`.

Define the prepared record or predicate shape for dependency operands of edge
source producers, including:

- edge and producer identity;
- operand value id/name/type;
- value-home/object linkage;
- cast/source identity when rematerialization is possible;
- explicit materialization policy and fail-closed statuses;
- freshness/clobber-safety and edge placement requirements.

If freshness/clobber-safety cannot be owned by this producer layer, Step 2
should route that blocker or lifecycle split before any implementation packet.

Future implementation proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Do not route to RV64 target lowering from current facts.
- Do not treat `%t17` stack home plus object metadata as sufficient
  `load_from_stack_slot` authority.
- Do not treat raw `inttoptr` plus `%t16` immediate as sufficient
  `rematerialize_cast_from_source` authority.
- Do not copy `%t18` from the successor/join block on the predecessor edge.
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
