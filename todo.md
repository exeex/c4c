Status: Active
Source Idea Path: ideas/open/507_rv64_select_publication_stack_home_materialization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Materialize GPR To Stack Destination

# Current Packet

## Just Finished

Step 3 implemented final RV64 materialization for the supported
select-publication direct GPR source to concrete stack-destination intent.

The consumer hook now admits the GPR-to-stack destination shape only when the
`EdgePublicationMoveIntent` is available, belongs to the matched
`SelectMaterialization` predecessor-terminator bundle, has a valid source GPR,
has matching scalar source/destination types, has concrete destination
stack-slot/offset/1-, 2-, or 4-byte-size fields, has a signed-12-bit stack
offset, and does not carry source immediate/source stack/memory/pointer
alternatives or a destination register. The materializer emits the store
through the existing RV64 stack-store helper.

Focused coverage in `backend_riscv_object_emission` now proves direct GPR
select-publication stack destinations emit `sb`, `sh`, and `sw` stores from
the published intent. The same test keeps large destination offsets and
stack-to-stack select-publication destinations fail-closed.

## Suggested Next

Execute Step 4 as the next lifecycle packet so the plan owner can decide
whether idea 507 closes after the supported source-home and destination-home
consumer paths, or whether a narrowed follow-up remains.

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
- Keep stack-to-stack select-publication unsupported without a dedicated
  scratch/interleaving policy.
- Keep generic immediate and stack-source-to-stack destination materialization
  outside idea 507 unless a separate source idea explicitly owns those routes.
- Preserve existing untracked review artifacts and the rejected
  `test_baseline.new.log`.

## Proof

Delegated Step 3 proof:

```sh
cmake --build build -j2 > test_after.log 2>&1 && ctest --test-dir build -j2 --output-on-failure -R '^backend_' >> test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. `test_after.log` reports `100% tests passed, 0 tests failed
out of 328`, followed by a clean `git diff --check`.
