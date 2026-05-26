Status: Active
Source Idea Path: ideas/open/04_aarch64_prior_preservation_baseline_drift.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Prove the Focused Prior-Preservation Path

# Current Packet

## Just Finished

Step 1: Reconstruct Close-Proof Scope completed as a validation and
close-readiness audit only. No implementation work is already required by the
source idea beyond proving the repaired AArch64 prior-preservation and
source-selection path and handing the result back for lifecycle close
handling.

The representative focused tests are present in the current CTest inventory:
`backend_aarch64_instruction_dispatch`,
`c_testsuite_aarch64_backend_src_00173_c`,
`c_testsuite_aarch64_backend_src_00179_c`,
`c_testsuite_aarch64_backend_src_00186_c`, and
`c_testsuite_aarch64_backend_src_00187_c`.

The historical baseline scope is reconstructed as:

- `log/baseline_08a7f725a3f5820506a517f58ae9b9012bc20b7e.log` is clean:
  3410/3410 tests passed.
- `log/baseline_11b33c8d0586b44d163de6b74bff9c33957aab0b.log` has 37
  failures, including `00173`, `00179`, `00186`, and `00187`.
- Current canonical logs must be regenerated for this active plan. The
  existing `test_after.log` belongs to the prior dispatch-materialization
  route and is not reusable close proof for this plan.

## Suggested Next

Step 2 packet: run the supervisor-selected focused proof and write the build
plus focused CTest output to the canonical `test_after.log` path:

```sh
cmake --build --preset default > test_after.log 2>&1
ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00173_c|c_testsuite_aarch64_backend_src_00179_c|c_testsuite_aarch64_backend_src_00186_c|c_testsuite_aarch64_backend_src_00187_c)$' >> test_after.log 2>&1
```

## Watchouts

- Treat this as validation and closure-readiness, not fresh implementation.
- Do not revive the retired broad prior-preservation fallback.
- Do not weaken c_testsuite expectations or mark tests unsupported.
- Keep `00181`, `00216`, and `00204` classified as separate families unless
  fresh proof shows they are actually prior-preservation/source-selection drift.
- Existing `test_after.log` does belong to another active route, so Step 2 must
  regenerate it before any close claim.
- Step 3 should use close-grade validation after the focused proof is green:
  prefer matching canonical regression evidence with `test_before.log` and
  `test_after.log` plus a supervisor-selected broader guard such as
  `c4c-regression-guard`, full CTest, or a broader AArch64/backend subset.

## Proof

Step 1 audit commands run:

```sh
ctest --test-dir build -N -R '^(backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_00173_c|c_testsuite_aarch64_backend_src_00179_c|c_testsuite_aarch64_backend_src_00186_c|c_testsuite_aarch64_backend_src_00187_c)$'
rg -n "(100% tests passed|Failed|failed|backend_aarch64_instruction_dispatch|c_testsuite_aarch64_backend_src_0017[39]_c|c_testsuite_aarch64_backend_src_0018[67]_c|3410|37)" log/baseline_08a7f725a3f5820506a517f58ae9b9012bc20b7e.log log/baseline_11b33c8d0586b44d163de6b74bff9c33957aab0b.log
```

Formatting proof for this packet passed:

```sh
git diff --check -- todo.md
```

No Step 1 `test_after.log` was generated; Step 2 owns regenerating it with the
focused proof command above.
