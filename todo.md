Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Deactivate Or Switch From The Umbrella

# Current Packet

## Just Finished

Step 3: Select The Next Focused Owner.

Selected the next focused owner from the committed Step 2 classification:
`ideas/open/348_aarch64_indexed_aggregate_address_writeback.md`.

Selection rationale:

- The strongest ready bucket is indexed aggregate addressing/writeback:
  `00130`, `00176`, `00182`, `00187`, `00195`, with `00181` as a nearby
  crash whose generated shape mutates global tower arrays through pointer
  parameters using fixed global snapshots and address-selected stores.
- This selection is based on generated-code first-bad-fact evidence, not
  failing counts: wrong local byte offset for `arr[1][3]`, unchanged global
  quicksort array after indexed swaps, lost LED buffer digits, a local buffer
  terminator landing at the wrong byte, repeated stores of stale `d9` while
  walking `point_array+N`, and recursive pointer/array mutation through fixed
  global element snapshots.
- Existing open parked ideas do not fit this evidence. The open aggregate
  ideas are scoped to variadic/byval/`va_arg` call-boundary behavior, idea 316
  is parked until fresh frame-size/frame-slot divergence evidence returns, and
  the scalar-cast/publication ideas explicitly do not own broad aggregate
  runtime value correctness.
- The boolean/comparison bucket remains viable but less ready as one owner:
  `00112`, `00119`, `00123`, and `00200` mix pointer/string-null comparison,
  FP comparison lowering, and a broad return-after-call materialization shape.
  It should stay parked until a later classification pass narrows its first
  generated-code owner.
- Other buckets remain parked or quarantined: `00005` admission,
  `00020`/`00103` pointer indirection, `00168` recursive scalar state,
  `00174` FP expression/call lowering, `00205`/`00216` static aggregate
  initializer/relocation materialization, `00173`/`00207` timeouts, and
  unresolved `00186` loop/update state.

No lifecycle switch was performed in this packet. The umbrella plan remains
active so Step 4 can deactivate or switch to idea 348 under lifecycle
authority.

## Suggested Next

Step 4 should switch lifecycle state from the umbrella inventory to
`ideas/open/348_aarch64_indexed_aggregate_address_writeback.md` before any
implementation work starts, unless the supervisor chooses to review or amend
the owner decision first.

## Watchouts

- Do not implement fixes under the umbrella inventory plan.
- Do not reopen closed or parked owners from failing counts alone.
- Do not fold the boolean/comparison bucket into idea 348 without fresh
  generated-code evidence tying those first bad facts to dynamic indexed
  aggregate address/writeback.
- The delegated command wrote only the CTest portion to `test_after.log`; the
  preceding build completed separately with `ninja: no work to do`.
- The shell pipeline exited successfully because `tee` was last in the pipeline,
  but CTest reported residual backend failures in the captured log.
- Local backend/unit/CLI tests are clean in this inventory; all residuals are
  external `c_testsuite_aarch64_backend_*` cases.
- Some runtime buckets may collapse after the first semantic owner is fixed:
  `00130`, `00176`, `00181`, `00182`, `00187`, and `00195` all show aggregate
  indexing/writeback symptoms and should not be split by testcase shape.
- Keep timeout cases `00173.c` and `00207.c` bounded before trusting broad
  runtime logs. No timeout reproduction was run in this packet.
- `00186.c` is explicitly accounted for as unresolved classification evidence
  above; do not treat it as a separate implementation owner until Step 3
  confirms whether it belongs with loop/update state or aggregate writeback.

## Proof

No rerun by delegation. Owner selection used the committed Step 2
classification and the already captured Step 1 proof:

`cmake --build --preset default && ctest --test-dir build -j10 -R backend --output-on-failure | tee test_after.log`

Read-only evidence sources: `test_after.log`,
`build/Testing/Temporary/LastTest.log`, and generated assembly files under
`build/c_testsuite_aarch64_backend/src/*.c.s`. No root-level logs were modified.
