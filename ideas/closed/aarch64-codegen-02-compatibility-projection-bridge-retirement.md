# AArch64 Compatibility Projection Bridge Retirement

## Intent

Retire or contract `compatibility_projection.*` only where selected target
records fully own the behavior currently projected through the compatibility
bridge.

This idea is explicitly blocked unless the implementation can prove that the
selected target records already carry every fact needed by the legacy projected
records for the chosen scope.

## Target Files

- `src/backend/mir/aarch64/codegen/compatibility_projection.cpp`
- `src/backend/mir/aarch64/codegen/compatibility_projection.hpp`
- `src/backend/mir/aarch64/codegen/module_compile.cpp` as call-site proof
  context only.

Do not use this idea to redesign module compilation or selected target records.

## Refactor Type

`Bridge retirement`

Use exactly this slice type. Do not combine it with contract redesign, phase
extraction, broad renaming, or generic lowering cleanup.

## Durable Owner

Selected target records are the durable owner. The compatibility projection is
only a bridge from selected target records to legacy projection records.

## Blocking Condition

Do not implement this idea until the selected target records fully own the
projected behavior for the selected scope.

If any object record, global record, unsupported-node report, diagnostic path,
or module-level behavior still depends on the legacy projection surface as a
source of truth, stop and leave the bridge in place.

## Scope

- Pick one small projected-record family or one narrow bridge entry point.
- Prove that selected target records already carry the required facts.
- Remove, contract, or localize only the compatibility glue that became
  redundant.
- Keep `module_compile.cpp` changes limited to using the already-owned selected
  target record behavior.

## Behavior-Preservation Proof

Expected proof for a future implementation run:

- build proof for the AArch64 backend target
- focused module/AArch64 compatibility tests covering:
  - selected target records
  - unsupported-node reporting
  - object record projection
  - global record projection

The proof must show that the newer contract owns the behavior before bridge
retirement is accepted.

## Reject Signals

Reject the implementation route if it requires any of the following:

- expectation downgrades or unsupported-test conversions
- testcase-shaped shortcuts or named-case matching
- hidden semantic changes to module compilation, object/global records, or
  unsupported-node reporting
- changing the selected target record contract to force the retirement
- target-specific instruction or register logic moved into generic layers
- giant multi-purpose refactors or module pipeline redesign
- broad renames without durable concept proof
- reducing file count while increasing responsibility mixing

## Acceptance

- A narrow compatibility bridge path is retired, contracted, or proven still
  necessary and left in place.
- Selected target records are the only accepted source of truth for any retired
  projected behavior.
- The slice is small enough for one focused future run.

## Closure

Closed after localizing the selected non-return machine-node filtering rule to
`module::selected_machine_nodes`. The broader `FunctionRecord` /
`CompatibilityProjection` wrapper, prepared-name label behavior, object/global
projection, and diagnostic ownership paths remain intentionally out of scope
because the ownership gate did not clear them.

Close proof used matching backend logs: `test_before.log` and `test_after.log`
both reported 162 passed, 0 failed, and the monotonic regression guard passed
with non-decreasing pass-count policy.
