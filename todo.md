Status: Active
Source Idea Path: ideas/open/305_aarch64_ctestsuite_00205_value_materialization_residual.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair Or Split The Value Owner

# Current Packet

## Just Finished

Step 1 - Classify Mismatch Mechanism: classified the `00205` wrong-output
mechanism as missing final AArch64 materialization for prepared direct-global
loads feeding call arguments, not frame-size accounting. Semantic and prepared
BIR contain correct dynamic global scalar materialization for `cases`: e.g.
`%t26.* = bir.load_global i64 @cases` at offsets `0..472`, `%t34.*` at
`32..496`, `%t42.*` at `40..496`, and `%t50.*` at `48..496`; prepared
addressing records those loads as `base=global_symbol symbol=cases`. Regalloc
then assigns the selected values to spill homes such as `%t26` at stack+632,
`%t34` at stack+1064, `%t42` at stack+1352, and `%t50` at stack+1496. Final
assembly never emits `adrp/add/ldr` from `cases` for those values and instead
passes call arguments by reading `[sp, #632]`, `[sp, #1064]`, `[sp, #1352]`,
and `[sp, #1496]`. Those high `[sp, #...]` operands are storage-home offsets
for unmaterialized selected global-load results; they are not legitimate frame
object offsets and are outside the `sub sp, sp, #48` prologue. The printed
uniform garbage values are therefore uninitialized stack reads from missing
global-load/select materialization.

## Suggested Next

Delegate Step 2 to repair the AArch64 lowering path that must materialize
prepared `LoadGlobalInst` values with `base=global_symbol` into their selected
value homes, or pass them directly to call arguments, before their storage
slots are consumed. A semantic repair should cover dynamic global scalar-array
loads and selects generally; do not patch the named `00205` offsets.

## Watchouts

- Preserve the idea 304 timeout repair: `00205` should complete quickly and
  must not return to the 5-second timeout path.
- Preserve the idea 303 legality repair: generated assembly must keep legal
  sign-extension spelling such as `sxtw x9, w13`.
- The current final assembly still contains legal `sxtw x9, w13` and loop
  compares `cmp x9, #9` / `cmp x9, #4`.
- The BIR/prepared evidence says global addressing is preserved through
  prepared addressing; the break is after preparation, where the final AArch64
  body consumes stack homes for values that were never written.
- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, proof-log policy, or CTest registration.
- Do not special-case filename `00205`, exact stack offsets, case indexes, or
  temporary labels.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'c_testsuite_aarch64_backend_src_00(064|139|205)_c' > test_after.log 2>&1`.
The build had no work; CTest returned nonzero because `00205` still has a
runtime output mismatch. `00064` and `00139` passed. `00205` completed in
0.07s, preserving the non-timeout behavior, and the generated assembly still
uses legal `sxtw x9, w13` spelling.
