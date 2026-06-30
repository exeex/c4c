Status: Active
Source Idea Path: ideas/open/452_select_edge_source_producer_rematerialization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Source-Producer Rematerialization Contract

# Current Packet

## Just Finished

Completed Step 2: defined the select-edge source-producer rematerialization
contract for idea 452. Supporting artifact:
`build/agent_state/452_step2_rematerialization_contract/contract.md`.

Accepted first implementation shape:

| Required fact | Accepted boundary |
| --- | --- |
| Edge publication | Available unique `select_materialization` edge publication matching predecessor, successor, destination value id, and out-of-SSA move. |
| Source producer | Prepared source-producer fact is `binary` with explicit producer block and instruction index. |
| Producer op | Successor/join-block pointer compare in the supported compare family. |
| Operand availability | Compare operands are materializable at the predecessor edge from prepared register homes or accepted immediates. |
| Destination | Selected destination has an available register-destination block-entry publication and GPR-compatible home. |
| Emission rule | Rematerialize the compare into the destination register; do not copy the successor-block compare result register. |

Rejected/routed from the first packet: `%t18 -> %t22`, because `%t18`
depends on `%t17 = inttoptr i32 %t16 to ptr` and `%t17` has a stack-slot
pointer home. That shape remains fail-closed until a separate stack-slot
pointer dependency materialization contract exists or idea 451 provides the
needed authority.

## Suggested Next

Step 3 should implement or route `Rematerialize Register-Operand Select-Edge
Compare Sources`: consume prepared source-producer facts for the narrower
register-operand candidates `%t32 -> %t36` and `%t46 -> %t50`, while keeping
the `%t18/%t17` stack-slot pointer dependency rejected from the first packet.

Suggested Step 3 owned files:

- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`
- `test_after.log`
- `build/agent_state/452_step3_select_edge_rematerialization/*`

Step 3 proof command:

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
- Do not admit stack-slot pointer dependencies in the first implementation
  packet; `%t18 -> %t22` remains the fail-closed boundary case.
- Step 3 must require prepared source-producer identity and cannot discover
  compare/cast dependencies from raw BIR alone.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, or `review/`.

## Proof

Step 2 contract-only validation:

```sh
git diff --check
```

Result: passed.
