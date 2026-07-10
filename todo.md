Status: Active
Source Idea Path: ideas/open/662_prepared_backend_contract_and_cli_publication.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Repair The Selected Contract Or CLI Rule

# Current Packet

## Just Finished

Step 3: Repair The Selected Contract Or CLI Rule hardened the selected
`backend_prepare_liveness` producer boundary for helper-built aggregate
`va_arg` AP pointer call-argument ABI destination publication.

Changed files:

- `src/backend/prealloc/regalloc/call_return_abi.cpp`
- `tests/backend/bir/backend_prepare_liveness_test.cpp`

The repair now teaches RV64 call-argument ABI register-index accounting that a
preceding sret-style pointer argument consumes one GPR argument slot only when
the argument being indexed is on the GPR lane. This preserves the helper-built
aggregate `va_arg` AP pointer destination at argument index 1, register `a1`,
while keeping unrelated FPR call-argument indexing on the FPR lane.

Added adjacent proof coverage in `backend_prepare_liveness_test.cpp`: an RV64
call with an sret-style pointer argument followed by an `f64` argument asserts
that the `f64` before-call ABI binding remains register `fa0`, so the sret slot
does not shift FPR argument indexing.

## Suggested Next

Delegate a supervisor-selected follow-up. The selected
`backend_prepare_liveness` row is green with adjacent FPR-lane risk covered;
the remaining stale f128 and CLI value-id/exposure rows from Step 1 should stay
separate unless the supervisor selects a new producer-evidence packet for one of
them. A reasonable next selection-only packet is to refresh or review the
still-failing Step 1 rows after this hardening before choosing the next
implementation boundary.

Suggested focused proof for any recheck of this completed row remains
`ctest --test-dir build -j --output-on-failure -R '^backend_prepare_liveness$'`.

## Watchouts

- This packet did not edit expectations, unsupported markers, allowlists,
  runtime policy, baseline accounting, AArch64 dispatch, RISC-V object emission,
  or stale f128/CLI value-id rows.
- The code diff is intentionally limited to RV64 call-argument register index
  accounting for preceding sret-style pointer arguments.
- The added test is a focused adjacent positive assertion for RV64 FPR-lane
  non-interference, not an expectation downgrade.

## Proof

Delegated proof run and preserved in `test_after.log`:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_liveness$' > test_after.log 2>&1
```

Result: build completed and `backend_prepare_liveness` passed 1/1, including
the existing helper-built aggregate `va_arg` AP pointer check and the new RV64
sret-then-FPR non-interference assertion. Proof log: `test_after.log`.
