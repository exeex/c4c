Status: Active
Source Idea Path: ideas/open/576_rv64_pr56982_post_carrier_runtime_mismatch.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Repair the owner without changing carrier or route policy

# Current Packet

## Just Finished

Completed Step 4: repaired RV64 prepared object-emission call-argument handling
so a `SymbolAddress` argument is materialized from its object symbol before the
generic source-selection fallback can copy from a prior-preserved callee-saved
home. This keeps the later global symbol-address call argument on a fresh
PC-relative materialization path instead of `mv a0,s1`.

Changed files:
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`

## Suggested Next

Run the representative `src/pr56982.c` route again to confirm the post-carrier
runtime mismatch advances or disappears, then decide whether Step 5 needs a
route-specific follow-up or broader backend closure proof.

## Watchouts

- Do not change inline asm carrier diagnostics or unsupported classification.
- Do not use filename-specific handling for `src/pr56982.c`.
- Do not claim progress from expectation rewrites, unsupported-marker edits,
  allowlist changes, or runtime comparison changes.
- This repair intentionally prefers call-argument symbol rematerialization when
  the prepared argument encoding is `SymbolAddress`, even if source-selection
  metadata also says `PriorPreservation`.
- Carrier diagnostics, route policy, allowlists, and expected-output markers
  were not touched.

## Proof

Step 4 delegated proof:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'; } > test_after.log 2>&1`

Result: passed. The focused `backend_riscv_object_emission` subset passed with
the new symbol-address prior-preservation proof. Proof log path:
`test_after.log`.
