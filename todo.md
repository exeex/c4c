Status: Active
Source Idea Path: ideas/open/457_before_instruction_stack_to_register_move_materialization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Register-Destination Move Materialization Contract

# Current Packet

## Just Finished

Completed Step 2 for idea 457. The register-destination move materialization
contract is recorded under
`build/agent_state/457_step2_register_destination_move_contract/contract.md`.

Accepted first packet shape: a narrow before-instruction compare-operand
materialization route, not generic stack-to-register support. Required facts:

- prepared `before_instruction` register-destination move bundle with stable
  block/instruction identity;
- explicit prepared source-producer for the same instruction;
- supported binary compare producer whose result is the destination value;
- operand-role mapping for every bundle move;
- GPR-compatible destination home;
- source operands with GPR register homes, supported immediates, or explicit
  `rematerialize_cast_from_source status=available` authority;
- scratch/order proof that a source operand is not clobbered before compare
  emission, especially for same-destination bundles.

Rejected shapes:

- `load_from_stack_slot status=missing_stack_freshness`;
- ordinary `consumer_stack_to_register` without explicit rematerialization or
  freshness authority;
- same-destination bundles consumed as sequential ordinary moves;
- missing source-producer identity, operand-role mapping, source home,
  destination home, or GPR-compatible destination;
- stack-slot operands requiring generic stack-home branch operand support;
- unsupported cast/source widths or unsupported cast source homes;
- successor-result copies, raw `inttoptr` lowering, pointer provenance,
  expectation changes, or named-case matching.

## Suggested Next

Execute Step 3: `Implement Or Route First Move-Bundle Packet`.

If the current prepared/RV64 event surfaces can expose the required facts,
implement the smallest semantic consumer for the accepted
before-instruction compare-operand materialization route. The representative
class is `%t15` register-home LHS plus `%t17` RHS with explicit
`rematerialize_cast_from_source status=available`, producing `%t18` in a GPR,
with non-clobbering scratch/register order. If source-producer identity,
operand-role authority, source-home authority, or scratch/clobber safety cannot
be represented, record the exact blocker instead of broadening.

Suggested owned files for Step 3:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/prealloc/prepared_object_traversal.hpp` and
  `src/backend/prealloc/prepared_object_traversal.cpp` only if needed to expose
  the register-destination bundle event details
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp` only if a focused
  prepared fixture adjustment is needed
- `todo.md`
- `test_after.log`
- `build/agent_state/457_step3_register_destination_move_materialization/**`

Re-read:

- `ideas/open/457_before_instruction_stack_to_register_move_materialization.md`
- `build/agent_state/457_step1_before_instruction_register_destination_audit/audit.md`
- `build/agent_state/457_step2_register_destination_move_contract/contract.md`

Step 3 proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Do not reopen explicit cast-dependency authority consumption closed by idea
  456.
- Do not consume `load_from_stack_slot missing_stack_freshness`.
- Do not infer materialization authority from raw BIR shape, filenames,
  function names, block names, testcase shape, value ids alone, or one
  prepared dump.
- Keep general stack-home branch consumer work routed to
  `ideas/open/451_stack_home_branch_operand_materialization.md`.
- Keep pointer-value provenance and generic instruction-side lowering out of
  this plan.
- Step 3 must reject same-destination move bundles unless it can treat them as
  compare-operand setup with proven non-clobbering order; do not emit them as
  sequential ordinary copies.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, baseline logs, or
  `review/`.

## Proof

Step 2 contract-only validation:

```sh
git diff --check
```

Result: passed.
