Status: Active
Source Idea Path: ideas/open/449_pointer_relational_branch_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Pointer Relational Predicate Policy

# Current Packet

## Just Finished

Completed Step 2: defined the pointer relational predicate policy for idea 449
and recorded supporting notes under
`build/agent_state/449_step2_pointer_relational_policy/policy.md`.

Accepted first-packet predicates:

- `Ult`
- `Ule`
- `Ugt`
- `Uge`

Selected ordering semantics:

- Treat accepted pointer relational branches as numeric unsigned address-order
  comparisons in the RV64 object route, but only after prepared facts prove the
  condition, operands, labels, and homes are already target-consumable.
- This policy does not claim C same-object definedness or infer provenance. It
  preserves already-prepared semantic BIR relational branches for coherent
  register-home operands.

Required accepted facts:

- prepared conditional branch terminator;
- matching `PreparedBranchCondition` for the same block;
- `kind=fused_compare`;
- `compare_type=Ptr`;
- predicate in `Ult/Ule/Ugt/Uge`;
- condition value equals the terminator condition;
- prepared true/false labels match terminator targets;
- condition value type `I32` with GPR-compatible prepared home;
- named pointer operands have GPR-compatible prepared homes;
- null pointer immediate operands are allowed;
- no target-side reconstruction from raw BIR compare/branch shape.

Rejected adjacent shapes:

- pointer `Eq`/`Ne`, closed by idea 441;
- signed pointer relational predicates (`Slt/Sle/Sgt/Sge`);
- select-result branch conditions, owned by idea 450;
- missing prepared branch condition;
- condition or target-label mismatch;
- missing or non-GPR-compatible condition/operand homes;
- stack-slot operand/condition homes in the first implementation packet;
- pointer-value memory operands with unknown range/layout/provenance;
- non-null pointer immediates until explicit producer authority exists;
- external-linkage formal provenance claims such as `930930-1` without durable
  no-external-caller authority;
- expectation/allowlist/unsupported-marker/runtime comparison/pass-fail
  accounting changes.

First implementation candidate:

- `20010329-1 main.entry`-style `branch_condition ... compare=uge ptr %t5, %t7`
  with GPR homes for `%t5`, `%t7`, and `%t8`.

## Suggested Next

Execute Step 3: implement or route the first unsigned pointer relational branch
packet.

Suggested owned files/tests:

- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/bir/backend_prepare_stack_layout_test.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`
- `test_after.log`
- `build/agent_state/449_step3_unsigned_pointer_relational_branch/*`

Exact Step 3 proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Keep this plan limited to pointer relational fused branch predicates.
- Do not reopen pointer `eq/ne` branch publication; closed idea 441 owns that
  route.
- Do not fold select-result branch publication into this plan; that belongs to
  `ideas/open/450_select_result_branch_publication.md`.
- Do not infer pointer relational semantics from raw BIR compare/branch shape,
  block order, filenames, function names, testcase names, or one dump layout.
- Keep missing, incoherent, unsupported, or policy-ineligible relational branch
  authority fail-closed.
- Step 3 may target `20010329-1 main.entry`-class unsigned address-order
  relational branches with GPR homes, but must remain semantic and not keyed to
  testcase/function/block names.
- Keep `930930-1` stack-home and pointer-value-memory rows out of the first
  consumer packet.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, or `review/`.

## Proof

Activation-only validation:

```sh
git diff --check
```

Step 1 classification validation:

```sh
git diff --check
```

Result: passed.

Step 2 policy validation:

```sh
git diff --check
```

Result: passed.
