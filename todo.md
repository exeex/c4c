Status: Active
Source Idea Path: ideas/open/552_prepared_move_bundle_target_shape_authority_gaps.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repair Scalar Type, Size, And Alignment Authority

# Current Packet

## Just Finished

Step 4, "Repair Scalar Type, Size, And Alignment Authority," completed as a
narrow implementation packet. Initial `git status --short` was clean.
Prepared RV64 register homes now publish scalar size and alignment facts from
the owning `PreparedRegallocValue::type` for ordinary RV64 register-memory
scalar types. The rule is target-scoped to RV64 and routes out `I128`, `F128`,
`Void`, and vector modes instead of making scalar policy here.

Changed files:

- `src/backend/prealloc/regalloc/value_homes.cpp`
- `docs/rv64_gcc_torture_post_contract/prepared_authority_scalar_type_size_alignment_step4.md`
- `todo.md`

Derived artifacts:

- `build/agent_state/552_step4_scalar_type_size_alignment.allowlist`
- `build/agent_state/552_step4_scalar_type_size_alignment/summary.tsv`
- `build/agent_state/552_step4_scalar_type_size_alignment/failed.txt`
- `build/agent_state/552_step4_scalar_type_size_alignment/row_status.tsv`

Focused Step 4 row counts:

| Classification | Rows |
| --- | ---: |
| Advanced to later RV64 instruction-fragment diagnostic | 10 |
| Still scalar type/size/alignment authority gap | 0 |

All focused rows now report `unsupported_instruction_fragment` instead of
`unsupported_move_bundle_target_shape`: `src/20020402-1.c`,
`src/20050215-1.c`, `src/950710-1.c`, `src/990811-1.c`, `src/loop-1.c`,
`src/loop-2d.c`, `src/pr42142.c`, `src/pr51466.c`, `src/pr62151.c`, and
`src/pr36339.c`. The remaining owner is RV64 instruction-fragment lowering,
not prepared scalar type/size/alignment authority.

## Suggested Next

Executor should run Step 5, "Repair Return ABI And Select-Publication
Authority," covering `prepared_return_abi_destination_home_authority` and
`prepared_select_publication_source_home_authority` rows.

## Watchouts

- The focused Step 4 allowlist still fails `0/10` by pass count, but every row
  advanced to `unsupported_instruction_fragment`; do not treat those rows as
  still owned by prepared scalar authority.
- The first draft of this packet published register size/alignment generically
  and regressed an AArch64 byval contract. The final rule is RV64-scoped.
- Do not pull `I128` or `F128` scalar policy into this prepared-authority
  packet.

## Proof

```text
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '^backend_'
ALLOWLIST=build/agent_state/552_step4_scalar_type_size_alignment.allowlist scripts/check_progress_rv64_gcc_c_torture_backend.sh
```

Results:

- Build passed.
- Backend CTest passed `345/345`.
- Focused Step 4 proof passed `0/10`, with `0` rows still blocked by scalar
  type/size/alignment authority and all 10 rows advanced to later
  `unsupported_instruction_fragment`.

Proof output is preserved in `test_after.log`.
