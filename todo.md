Status: Active
Source Idea Path: ideas/open/306_aarch64_symbol_offset_address_materialization_width.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Address Register Width For Symbol+Offset Memory References

# Current Packet

## Just Finished

Step 2 completed the symbol+offset address-width repair in
`src/backend/mir/aarch64/codegen/alu.cpp`.

`append_move_control_value_to_register` now materializes
`MemoryBaseKind::Symbol` addresses through an `X` scratch/base register and
loads the scalar data into the requested `target` view, so I32 scalar data still
lands in `W` registers while `[base]` uses an address-width register.

The sibling control-source materializer in the same control-publication path
was repaired the same way after the first proof run showed `00050.c` still
emitted the comparison-side `adrp w9` / `[w9]` sequence. The current generated
assembly for `00050.c`, `00176.c`, and `00182.c` has no `adrp w...` or `[wN]`
symbol+offset memory bases.

## Suggested Next

Use the supervisor-selected next packet to investigate the remaining
non-width residuals exposed by the proof: `00176.c` now executes but still
produces the unsorted second line, and `00182.c` still fails assembly on
`mov x0, #1234567`.

## Watchouts

- The first post-edit proof moved `00050.c` from the known invalid `adrp w9`
  sequence to passing after the sibling compare materializer was repaired.
- `00176.c` changed from the baseline backend failure caused by illegal `w`
  address materialization to a runtime mismatch. Do not treat the current
  runtime mismatch as proof that the address-width repair is incomplete without
  a fresh semantic owner trace.
- `00182.c` still has the same baseline large-immediate assembly failure
  (`mov x0, #1234567`), which is separate from symbol+offset address width.

## Proof

Ran exactly:

`cmake --build --preset default && (ctest --test-dir build -j --output-on-failure -R '^c_testsuite_aarch64_backend_src_(00050|00176|00182)_c$') 2>&1 | tee test_after.log; exit ${PIPESTATUS[0]}`

Result: build passed; test subset failed. `00050.c` passed. `00176.c` failed
with a runtime mismatch after the illegal address-width assembly was removed.
`00182.c` failed on the pre-existing large-immediate assembly error
`mov x0, #1234567`. Supervisor regression guard compared the matching focused
`test_before.log` / `test_after.log` pair and passed: 0/3 to 1/3, no new
failures. The accepted after log was rolled forward to canonical
`test_before.log`.
