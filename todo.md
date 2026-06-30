Status: Active
Source Idea Path: ideas/open/470_branch_stack_load_policy_freshness.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Policy Freshness Contract

# Current Packet

## Just Finished

Completed Step 2 contract for idea 470. The first accepted shape is bounded to
scalar branch-condition records that already reach `missing_policy` with
coherent branch/value/home/frame-slot/object facts.

Positive contract:

| Fact group | Required facts |
| --- | --- |
| Branch identity | Prepared branch condition and matching BIR conditional branch terminator agree on condition and targets. |
| Role | `condition` only for the first packet. |
| Value/home/object | Named scalar condition, stack-slot home, frame slot, and stack object all match by value id/name, slot/object id, offset, size, and alignment. |
| Policy | Explicit `load_from_stack_slot`; `none` remains unavailable. |
| Freshness | Explicit proof that the slot contains the current condition value at the branch site. |
| Clobber safety | Explicit proof that no store/call/helper/publication/move/stack write can clobber the slot before the branch load. |
| Pointer status | `not_pointer` only for the first positive scalar condition class. |

Rejected/fail-closed shapes:

| Shape | Disposition |
| --- | --- |
| `unsupported_terminator` rows (`f.block_1`, `f.block_4`) | Keep fail-closed until branch-site relationship acceptance is separately proven. |
| Pointer operands `%t1` / `%t7` | Keep fail-closed unless pointer/provenance status is explicit; not in first packet. |
| Select-result stack destination `%t22` | Keep separate; do not accept as first scalar condition policy proof. |
| Lhs/rhs operands | Out of first positive packet. |
| Missing policy/freshness/clobber safety | Must remain explicit unavailable statuses. |
| Raw-shape-only evidence | Rejected. Stack-slot spelling, offsets, block labels, names, and testcase shape are not authority. |

Selected Step 3 packet: `Populate Scalar Condition Branch-Stack-Load Policy`.
Implement only producer/prepared population for scalar condition rows that
already have coherent branch/value/home/frame-slot/object facts; route/block if
freshness or clobber-safety cannot be proven without raw-shape inference.

Artifact:
`build/agent_state/470_step2_policy_freshness_contract/contract.md`.

## Suggested Next

Execute Step 3: Implement Or Route Policy Freshness Packet.

Suggested owned files:

- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/prepared_printer/**` only if field/status exposure
  needs a small adjustment
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
- `tests/backend/bir/backend_prepared_printer_test.cpp` only if printer
  coverage changes
- `todo.md`
- `test_after.log`
- `build/agent_state/470_step3_policy_freshness_population/**`

Proof:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Do not edit RV64 target lowering.
- Do not set `load_from_stack_slot` unless freshness and clobber safety are
  also explicitly proven.
- Preserve `unsupported_terminator` rows as unavailable unless a separate
  prerequisite packet proves the branch-site relationship.
- Do not implement RV64 branch-load emission in this producer plan.
- Do not accept unavailable `PreparedBranchStackLoadAuthority` records as
  target authority.
- Do not infer load authority from stack homes, offsets, object ids, raw BIR,
  block labels, function names, or testcase shape.
- Keep pointer-value/provenance and select-result stack-destination boundaries
  separate.
- Do not modify `test_baseline.new.log`, `test_baseline.log`,
  `test_before.log`, `test_after.log`, or `review/`.

## Proof

Contract proof:

```sh
git diff --check
```

Result: passed.
