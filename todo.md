Status: Active
Source Idea Path: ideas/open/507_rv64_select_publication_stack_home_materialization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Materialize Pointer Stack-Source To GPR

# Current Packet

## Just Finished

Step 2 implemented final RV64 materialization for the supported
select-publication pointer/XLEN concrete stack-source to GPR intent only.

The consumer hook now admits a select-publication move only when the
`EdgePublicationMoveIntent` is available, belongs to the matched
`SelectMaterialization` predecessor-terminator bundle, has pointer source and
destination types, has concrete source stack-slot/offset/8-byte-size fields,
uses a valid GPR destination register, has a signed-12-bit stack offset, and
does not carry source register/immediate/memory/pointer alternatives or
destination stack fields. The materializer emits the load through the existing
RV64 stack-load helper.

Focused coverage in `backend_riscv_object_emission` now proves the
`src/builtin-constant.c` representative shape emits `ld a0, 8(sp)` from the
published intent. The same test keeps a large-offset pointer stack-source case
fail-closed and preserves the existing stack-destination representative as
intent-only, not final object admission.

## Suggested Next

Execute Step 3 as the next bounded consumer packet for the supported
select-publication GPR-to-stack destination intent from `src/pr58726.c`, if the
plan owner keeps idea 507 open for stack-destination final materialization.
Keep generic immediate materialization for `src/pr37924.c` routed to idea 506.

## Watchouts

- Keep `src/pr37924.c` routed to idea 506; it is generic out-of-SSA immediate
  materialization, not select-publication stack-home work.
- Preserve fail-closed behavior for missing/non-available intent, publication
  or bundle mismatch, non-predecessor execution block, cycle/temp/non-move
  steps, missing concrete stack fields, non-GPR register identities, large
  offsets, unsupported widths, and stack-to-stack select-publication without a
  scratch/interleaving policy.
- Do not fold generic immediate materialization from `src/pr37924.c` into idea
  507.
- Do not infer homes or authority from testcase names, raw BIR shape, object
  output, final registers, target behavior, or absent diagnostic tokens.
- Keep stack-destination and stack-to-stack select-publication unsupported
  without dedicated Step 3 admission/materialization policy.
- Preserve existing untracked review artifacts and the rejected
  `test_baseline.new.log`.

## Proof

Delegated Step 2 proof:

```sh
cmake --build build -j2 > test_after.log 2>&1 && ctest --test-dir build -j2 --output-on-failure -R '^backend_' >> test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. `test_after.log` reports `100% tests passed, 0 tests failed
out of 328`, followed by a clean `git diff --check`.
