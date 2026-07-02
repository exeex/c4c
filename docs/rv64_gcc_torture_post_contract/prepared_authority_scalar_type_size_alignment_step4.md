# Prepared Authority Step 4 Scalar Type, Size, And Alignment Repair

Status: Step 4 implementation packet for
`ideas/open/552_prepared_move_bundle_target_shape_authority_gaps.md`.

## Rule

Prepared RV64 register homes now publish scalar size and alignment facts from
the owning `PreparedRegallocValue::type` when the type has ordinary RV64
register-memory authority. This gives prepared move-bundle consumers explicit
source storage width for register-to-stack moves without inferring it from
RV64 final shape or diagnostics.

The rule is target-scoped to RV64 and deliberately routes out `I128`, `F128`,
`Void`, and vector register modes. It does not change AArch64 or x86 prepared
home publication, expectation files, unsupported markers, runtime comparison,
or RV64 lowering policy.

## Focused Rows

The Step 4 allowlist contains the 9 scalar-type rows from the Step 1 queue
plus the Step 2 carry-forward `src/pr36339.c`:

- `build/agent_state/552_step4_scalar_type_size_alignment.allowlist`
- `build/agent_state/552_step4_scalar_type_size_alignment/summary.tsv`
- `build/agent_state/552_step4_scalar_type_size_alignment/failed.txt`
- `build/agent_state/552_step4_scalar_type_size_alignment/row_status.tsv`
- Proof log: `test_after.log`

## Results

Focused proof result:

```text
[rv64-gcc-torture] total=10 passed=0 failed=10
```

Row classifications after the repair:

| Classification | Rows |
| --- | ---: |
| Advanced to later RV64 instruction-fragment diagnostic | 10 |
| Still scalar type/size/alignment authority gap | 0 |

All focused rows now report `unsupported_instruction_fragment` rather than
`unsupported_move_bundle_target_shape`, so they no longer need prepared scalar
type/size/alignment authority. Their remaining owner is RV64 instruction
fragment lowering, not this prepared-authority plan.

## Proof

```text
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '^backend_'
ALLOWLIST=build/agent_state/552_step4_scalar_type_size_alignment.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

Results:

- Build passed.
- Backend CTest passed `345/345`.
- Focused allowlist passed `0/10`, with all 10 rows advanced to
  `unsupported_instruction_fragment` and 0 rows still blocked by scalar
  type/size/alignment authority.

Proof output is preserved in `test_after.log`.
