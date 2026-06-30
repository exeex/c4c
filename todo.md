Status: Active
Source Idea Path: ideas/open/456_rv64_select_edge_cast_dependency_consumer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Cast Dependency Consumer Evidence

# Current Packet

## Just Finished

Completed Step 1 for idea 456. Audit artifact:
`build/agent_state/456_step1_cast_dependency_consumer_audit/audit.md`.

Bucket table:

| Bucket | Edge identity | Source producer | Operand role | Dependency value | Cast producer/source | Policy/status | RV64 failure | Classification |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| First bounded consumer candidate | `main`, `logic.rhs.end.13 -> logic.end.14`, `%t18 -> %t22` | `binary`, `logic.end.14` inst `2`, `%t18 = ule ptr %t15, %t17` | `rhs` | `%t17`, value id `9`, stack slot `#2`, offset `16` | `%t17 = inttoptr i32 %t16`; `%t16` value id `8`, home `rematerializable_immediate`, imm `-2147483643` | `rematerialize_cast_from_source`, `available` | `unsupported_move_bundle_target_shape` | Coherent RV64 consumer packet candidate |
| Stack-load alternative | Same edge and `%t17` dependency | Same source producer | `rhs` | `%t17` stack slot `#2` | Not required | `load_from_stack_slot`, `missing_stack_freshness` | Same failure | Fail-closed; no freshness/clobber authority |
| Successor result copy | Same edge source `%t18` | `%t18` defined in successor block `logic.end.14` | N/A | N/A | N/A | No copy authority | Same failure | Rejected as unsound |
| Adjacent register/immediate select-edge compares | Later `%t32 -> %t36` / `%t46 -> %t50` classes | Binary compare with target-consumable operands | Existing operands | Register/immediate homes | No stack-slot cast dependency | Already validated by idea 452 | Not current first owner | Out of scope for Step 2 |

Fresh probes under
`build/agent_state/456_step1_cast_dependency_consumer_audit/`:

- prepared route exits 0 and prints the available cast-rematerialization record;
- object route exits 2 with
  `unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves`.

## Suggested Next

Step 2: `Define RV64 Cast Dependency Consumer Contract`.

Define the bounded RV64 consumer contract for explicit
`rematerialize_cast_from_source status=available` dependency-operand authority:
matching edge/source-producer/operand/dependency/cast/source-home facts,
materialization order, temporary/register requirements, supported cast widths,
and fail-closed adjacent shapes. The later implementation packet should be
semantic and should not consume stack-load, raw `inttoptr`, or successor-copy
evidence.

Future proof command for implementation:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Consume only populated `rematerialize_cast_from_source status=available`
  dependency-operand authority.
- Do not infer from raw `inttoptr`, stack homes, object metadata, filenames,
  function names, block names, or one prepared dump.
- Do not consume `load_from_stack_slot` while the row is
  `missing_stack_freshness`.
- Do not copy `%t18` from the successor/join block on the predecessor edge.
- Keep general stack-home branch consumer work routed to
  `ideas/open/451_stack_home_branch_operand_materialization.md`.
- Keep pointer-value provenance and generic instruction-side lowering out of
  this plan.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 1 proof:

```sh
git diff --check
```

Result: passed.
