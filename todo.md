Status: Active
Source Idea Path: ideas/open/664_riscv_object_emission_internal_probe.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Regression Safety And Lifecycle Readiness

# Current Packet

## Just Finished

Step 4 of `plan.md` repaired the prepared prior-preserved argument direct
extern call object path in
`src/backend/mir/riscv/codegen/object_emission.cpp`.

`fragment_for_prepared_call()` now resolves prior-preserved GPR call-argument
sources through the shared indexed prior-preservation lookup and still checks
that the selected producer position matches the source selection. This keeps
ambiguous, invalid, or mismatched preservation shapes fail-closed while allowing
the second direct `probe` call in
`make_prepared_prior_preserved_arg_call_module()` to use the earlier preserved
value.

The object call result path also no longer suppresses after-call preservation
republication when the call-result move is a no-op (`a0` to `a0`). That restores
the expected preservation republication byte before the later prior-preserved
argument reload, so the fixture emits two `R_RISCV_CALL_PLT` relocations with
the expected call-preservation layout.

No tests, expectations, unsupported markers, allowlists, timeout/runtime policy,
baseline accounting, `plan.md`, or source idea files were edited.

## Suggested Next

Suggested next packet: classify the first remaining generic
`expected prepared RV64 object path to reject` in `test_after.log` with
gdb/backtrace before assigning ownership. Do not infer it is related to the
prior-preserved direct-call path; that row is now repaired.

## Watchouts

- `test_after.log` prints repeated generic messages without test names; use
  gdb/backtrace disambiguation before assigning ownership to any generic
  `expected prepared RV64 object path to reject` line.
- GDB inspection during this packet showed the repaired fixture now reaches two
  call relocations at offsets 20 and 36 after the after-call republication move;
  `test_after.log` no longer contains `expected two direct probe call
  relocations`.
- Later failures remain in the focused log, including generic rejection rows,
  byval/local-memory/sret rows, call-argument publication ordering, and
  diagnostic exactness. This packet did not absorb those.
- Do not rewrite expectations, unsupported markers, allowlists, timeout/runtime
  policy, or baseline accounting for this slice.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$' > test_after.log 2>&1`

Result: build passed; focused CTest still failed on later
`backend_riscv_object_emission` rows. `test_after.log` is the canonical proof
log and no longer contains the targeted
`expected two direct probe call relocations` failure or the follow-on
`expected preservation population/republication and later reload` failure.
