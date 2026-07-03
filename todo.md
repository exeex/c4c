# Current Packet

Status: Active
Source Idea Path: ideas/open/546_rv64_instruction_fragment_current_classification.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Classify Semantic Families And First Owners

## Just Finished

Step 2 (`Recover Or Refresh Authoritative Row Scope`) refreshed the full RV64
gcc_torture backend scan and regenerated the instruction-fragment row table
from that coherent run:

- Refreshed scan log:
  `build/agent_state/rv64_gcc_torture_backend_current_20260703T015523Z.log`.
- Updated scan pointer:
  `build/agent_state/rv64_gcc_torture_backend_current_log_path.txt`.
- Refreshed scan totals: `total=1467 passed=425 failed=1042`.
- Refreshed summary source:
  `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`.
- Accepted authoritative row table:
  `build/agent_state/unsupported_instruction_fragment_current_rows.tsv`.
- Accepted refreshed current count: `265` rows with
  `unsupported_instruction_fragment`.
- Derivation command: rerun
  `BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh`,
  then read fail rows from the refreshed summary TSV and keep only per-case
  logs from the same run containing `unsupported_instruction_fragment`.
- Wrote packet notes to
  `build/agent_state/546_step2_authoritative_row_scope.txt`.

## Suggested Next

Current packet: Step 3 (`Classify Semantic Families And First Owners`).
Classify the accepted coherent-run table
`build/agent_state/unsupported_instruction_fragment_current_rows.tsv` with its
`265` rows. Use
`build/agent_state/rv64_gcc_torture_backend_current_20260703T015523Z.log`,
`build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`, and the referenced
per-case logs as the provenance basis.

Expected output for this packet:

- A classification table or durable notes artifact named in `todo.md`.
- Sub-bucket counts for the 265 accepted rows.
- Representative cases for each recurring semantic family.
- First-owner rationale separating RV64 lowering gaps from producer,
  prepared-contract, ABI, F128, and evidence-gap rows.

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
- Screen primary-F128 rows into the quarantine lane before ordinary-C bucket ranking.

## Proof

- Evidence-only packet; no build proof required.
- Refresh command:
  `ts=$(date -u +%Y%m%dT%H%M%SZ); BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh >"build/agent_state/rv64_gcc_torture_backend_current_${ts}.log" 2>&1 || true; printf '%s\n' "build/agent_state/rv64_gcc_torture_backend_current_${ts}.log" > build/agent_state/rv64_gcc_torture_backend_current_log_path.txt`
- Validation: `git diff --check -- todo.md`
