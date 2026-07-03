# Current Packet

Status: Active
Source Idea Path: ideas/open/546_rv64_instruction_fragment_current_classification.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Screen F128 And Producer-Gap Rows

## Just Finished

Step 4 (`Screen F128 And Producer-Gap Rows`) screened all `265`
Step 3 rows from
`build/agent_state/546_step3_instruction_fragment_classification.tsv`.

Generated artifacts:

- Row-level screening table:
  `build/agent_state/546_step4_instruction_fragment_screening.tsv`.
- Screening notes:
  `build/agent_state/546_step4_instruction_fragment_screening.md`.

Screened-owner counts, summing to `265`:

- `producer_prepared_boundary`: `143`
- `implementation_ready_rv64`: `87`
- `f128_quarantine`: `28`
- `abi_or_call_boundary`: `5`
- `evidence_gap`: `2`

Representative examples:

- `src/20040709-2.c`, `src/930622-2.c`, `src/fprintf-chk-1.c`: F128 or
  long-double quarantine; do not mix into ordinary scalar RV64 progress.
- `src/20030408-1.c`, `src/pr51933.c`, `src/pr49279.c`,
  `src/20071211-1.c`: producer/prepared-contract boundary examples for
  join/select publication, local-memory, and global-addressing rows.
- `src/20001026-1.c`: implementation-ready integer div/rem row.
- `src/pr78438.c`: implementation-ready arithmetic shift-right row.
- `src/20000622-1.c`: implementation-ready pointer/integer cast row.
- `src/20040709-3.c`: implementation-ready F32/F64 scalar FP/cast row.
- `src/pr56982.c`: ABI/call helper boundary row.

Rationale: all `28` Step 3 `f128_or_long_double_primary` rows remain
quarantined. Rows with explicit `source_f128_or_long_double` or
`source_variadic` flags stay out of ordinary scalar work, and rows with no
source flag still have Step 3 F128-family evidence, so they fail closed into
the same lane. All `143` Step 3 `rv64_or_prepared_boundary` rows remain
producer/prepared-contract rows because joins, selects, local memory, and
global addressing need prepared authority before RV64 object lowering can be
claimed as first owner. The `87` provisional `rv64_object_lowering` rows remain
ordinary implementation-ready rows after screening: integer div/rem (`30`),
F32/F64 scalar FP/cast (`18`), scalar integer binary (`15`), arithmetic
shift-right (`12`), and pointer/integer cast (`12`).

## Suggested Next

Next packet: Step 5 (`Produce Follow-Up Routing`). Use
`build/agent_state/546_step4_instruction_fragment_screening.tsv` as the input.
Rank only the `87` `implementation_ready_rv64` rows for ordinary RV64/MIR
object-lowering follow-up and keep the `28` F128 rows, `143`
producer/prepared rows, `5` ABI/call rows, and `2` evidence-gap rows out of the
ordinary implementation queue.

## Watchouts

- Leave `review/557_step13_vector_local_memory_review.md` untouched.
- The refreshed coherent current count is `265`, not the source-expected `137`
  and not the Step 1 mixed-time `179`. Treat `265` as the accepted Step 3
  scope unless the supervisor asks for another refresh.
- Do not reuse stale or mixed-time artifacts as classification scope:
  `build/agent_state/unsupported_instruction_fragment_rows.tsv` (`190` rows
  from 2026-06-30), `build/agent_state/rv64_gcc_c_torture_backend_summary.full.tsv`
  (2026-07-01, reconstructs `179` rows against current logs), or the Step 1
  `179`-row mixed-time reconstruction.
- Do not implement RV64 lowering, edit expectations, or weaken unsupported markers in this classification packet.
- The Step 4 table is a screening/routing artifact, not an implementation
  proof. Boundary rows must not become RV64 implementation ideas until focused
  prepared-BIR evidence proves the prepared facts are complete.
- The `87` implementation-ready rows are rankable for follow-up, but Step 5
  still needs to recommend narrow buckets with concrete representative rows and
  reject testcase-shaped lowering.

## Proof

- Evidence-only packet; no CTest proof required.
- Screening input:
  `build/agent_state/546_step3_instruction_fragment_classification.tsv`.
- Screening outputs:
  `build/agent_state/546_step4_instruction_fragment_screening.tsv` and
  `build/agent_state/546_step4_instruction_fragment_screening.md`.
- Validation:
  - `git diff --check -- todo.md`
  - screening TSV data-row count is `265`
  - screened-owner and screening-decision totals each sum to `265`
