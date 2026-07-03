# Current Packet

Status: Active
Source Idea Path: ideas/open/546_rv64_instruction_fragment_current_classification.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Classify Semantic Families And First Owners

## Just Finished

Step 3 (`Classify Semantic Families And First Owners`) classified all `265`
accepted coherent-run `unsupported_instruction_fragment` rows from
`build/agent_state/unsupported_instruction_fragment_current_rows.tsv`.

Generated artifacts:

- Row-level classification table:
  `build/agent_state/546_step3_instruction_fragment_classification.tsv`.
- Representative notes and dump index:
  `build/agent_state/546_step3_instruction_fragment_representatives.md`.
- Focused BIR/prepared-BIR representative dumps:
  `build/agent_state/546_step3_*_{bir,prepared}.txt`.

Sub-bucket counts by semantic family, summing to `265`:

- `join_phi_branch_publication`: `56`
- `local_memory_or_stack_value`: `36`
- `global_memory_addressing_small`: `33`
- `integer_div_rem`: `30`
- `f128_or_long_double_primary`: `28`
- `scalar_fp_cast_or_op`: `18`
- `select_join_or_value_publication`: `18`
- `scalar_integer_binary`: `15`
- `integer_arithmetic_shift_right`: `12`
- `pointer_integer_cast`: `12`
- `call_adjacent_or_helper`: `5`
- `evidence_gap`: `2`

First-owner counts, also summing to `265`:

- `rv64_or_prepared_boundary`: `143`
- `rv64_object_lowering`: `87`
- `f128_quarantine`: `28`
- `abi_or_rv64_boundary`: `5`
- `evidence_gap`: `2`

Representative examples:

- `src/20030408-1.c`: join/phi/branch publication shape.
- `src/pr49279.c`: local-memory/stack-value boundary shape.
- `src/20071211-1.c`: small global-memory/addressing mixed shape.
- `src/20040709-3.c`: integer div/rem plus scalar FP conversion-heavy row.
- `src/20040709-2.c`: primary F128/long-double quarantine row.
- `src/pr51933.c`: high-volume select/join/value-publication row.
- `src/pr78438.c`: arithmetic shift-right row.
- `src/20000622-1.c`: pointer/integer cast row.

Rationale: rows with explicit scalar, pointer, integer-div/rem, shift, and
F32/F64 operation families are RV64 object-lowering candidates. Rows dominated
by select/join publication, local/global memory, or mixed call-adjacent shapes
remain boundary rows because the generic object-route diagnostic does not name
the exact first failing instruction; prepared authority must be checked before
implementation. Primary F128/long-double rows are flagged for quarantine rather
than ordinary scalar progress.

## Suggested Next

Next packet: Step 4 (`Screen F128 And Producer-Gap Rows`). Use
`build/agent_state/546_step3_instruction_fragment_classification.tsv` as the
input. Screen the `28` `f128_or_long_double_primary` rows in detail, then split
the `143` `rv64_or_prepared_boundary` rows into producer/prepared-contract
rows versus implementation-ready RV64 rows. Keep the `87`
`rv64_object_lowering` rows provisional until Step 4 confirms no obvious F128,
ABI, or producer-owned rows remain inside them.

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
- The Step 3 table is conservative semantic classification, not proof of the
  exact first failing instruction for every generic diagnostic row.
- Boundary rows must not become RV64 implementation ideas until focused
  prepared-BIR evidence proves the prepared facts are complete.

## Proof

- Evidence-only packet; no CTest proof required.
- Classification extraction:
  `build/c4cll --target riscv64-linux-gnu --dump-bir
  tests/c/external/gcc_torture/<case>` for each of the `265` accepted rows.
- Representative dump extraction:
  `build/c4cll --target riscv64-linux-gnu --dump-prepared-bir
  tests/c/external/gcc_torture/<case>` for one representative from each
  family.
- Validation:
  - `git diff --check -- todo.md`
  - classification TSV data-row count is `265`
  - family, first-owner, and representative-group totals each sum to `265`
