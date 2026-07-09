Status: Active
Source Idea Path: ideas/open/623_rv64_cast_instruction_fragment_consumers.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh cast residual evidence

# Current Packet

## Just Finished

Step 1 refreshed current cast-shaped `unsupported_instruction_fragment`
evidence from the RV64 gcc torture backend scan without implementation,
expectation, unsupported-marker, allowlist, runtime/accounting, or plan/idea
changes.

Refresh inputs and command provenance:
- Inputs: `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`,
  `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`, and
  `build/rv64_gcc_c_torture_backend/*/case.log`.
- Extraction classified only current failed-case diagnostics whose normalized
  text begins `prepared module shape: unsupported_instruction_fragment: BIR
  instruction requires unsupported RV64 object lowering` and exposes
  `instruction_kind=...`.
- Freshness boundary: the refresh uses the current scan artifacts dated
  2026-07-09T01:31:56Z, not stale idea-612 counts.

Artifacts written:
- `build/agent_state/623_step1_cast_residuals.tsv`: 28 current CastInst rows.
- `build/agent_state/623_step1_non_cast_guard_rows.tsv`: 60 current non-cast
  guard rows from the same diagnostic bucket.
- `build/agent_state/623_step1_cast_refresh_notes.md`: grouped counts,
  representative cast rows, representative non-cast guards, and the diagnostic
  boundary.

Current cast rows by source operation and diagnostic boundary:
- `CastInst: i32 ...`: 26 rows. Cases: `src/20010604-1.c`,
  `src/20020506-1.c`, `src/20021111-1.c`, `src/20030714-1.c`,
  `src/20090113-2.c`, `src/20090113-3.c`, `src/20120919-1.c`,
  `src/20150611-1.c`, `src/961122-2.c`, `src/compare-1.c`,
  `src/fprintf-chk-1.c`, `src/ieee/mzero5.c`, `src/loop-2d.c`,
  `src/p18298.c`, `src/pr19005.c`, `src/pr23467.c`, `src/pr37573.c`,
  `src/pr41750.c`, `src/pr43835.c`, `src/pr81555.c`, `src/pr81556.c`,
  `src/pr90949.c`, `src/printf-chk-1.c`, `src/stkalign.c`,
  `src/vfprintf-chk-1.c`, `src/vprintf-chk-1.c`.
- `CastInst: f128 ...`: 2 rows. Cases: `src/930622-2.c`,
  `src/ieee/pr29302-1.c`.
- Example diagnostic text for the row list is recorded verbatim per row in
  `623_step1_cast_residuals.tsv`; representative form:
  `prepared module shape: unsupported_instruction_fragment: BIR instruction
  requires unsupported RV64 object lowering; function=...; block=...;
  block_index=...; instruction_index=...; instruction_kind=CastInst;
  owner=...`.

Nearby non-cast guard rows from the same current diagnostic bucket:
- `BinaryInst`: 10 rows; mostly `ptr` owners plus one `i16 %t13.bf.sext`.
- `CallInst`: 39 rows; owners include `i32`, `ptr`, `double`, `float`, `i64`,
  and `none`.
- `SelectInst`: 7 rows; all representative rows are `ptr` owners.
- `StoreLocalInst`: 3 rows; float/double sret copy owners.
- `LoadLocalInst`: 1 row; `double %t10`.
- Full guard row details and representative examples live in
  `build/agent_state/623_step1_non_cast_guard_rows.tsv` and
  `build/agent_state/623_step1_cast_refresh_notes.md`.

## Suggested Next

Execute Step 2 from `plan.md`: bucket the 28 refreshed CastInst rows by first
missing or responsible owner, and identify whether a multi-row ordinary
RV64/MIR consumer sub-family exists after excluding `f128`/floating policy and
other non-consumer lanes.

## Watchouts

- Step 1 evidence is diagnostic classification only; it does not prove producer
  facts are complete for any CastInst row.
- Keep the 60 non-cast guard rows outside this route unless the supervisor
  opens a separate owner packet.
- The 2 `f128` CastInst rows should remain suspect policy/floating lanes until
  Step 2 proves otherwise.
- Reject named-case-only cast lowering and any expectation/unsupported-marker
  rewrite route.

## Proof

Supervisor-selected proof command:

```sh
cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^backend_' >> test_after.log 2>&1
```

Result: passed. `test_after.log` is the canonical proof artifact; the backend
CTest subset completed successfully.
