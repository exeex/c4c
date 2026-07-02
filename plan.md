# BIR Semantic Producer Admission Reconstruction Runbook

Status: Active
Source Idea: ideas/open/545_bir_semantic_producer_admission_reconstruction.md

## Purpose

Rebuild current row-level evidence for `semantic lir_to_bir` admission
failures before assigning implementation ownership or letting RV64/MIR consume
missing producer facts.

## Goal

Classify current BIR semantic admission failures into producer-owned families,
evidence gaps, or non-BIR owners, then create follow-up routing only from
current evidence.

## Core Rule

Do not claim BIR semantic producer progress from stale counts, downstream
RV64 workarounds, diagnostic rewrites, or weakened expectations. Current row
evidence must identify the first producer fact that is missing or malformed.

## Read First

- `ideas/open/545_bir_semantic_producer_admission_reconstruction.md`
- `docs/rv64_gcc_torture_post_contract/current_scan_summary.md`
- `docs/rv64_gcc_torture_post_contract/failure_bucket_map.md`
- `build/agent_state/rv64_gcc_torture_backend_current_log_path.txt`
- `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`
- `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`
- `build/rv64_gcc_c_torture_backend/<case-id>/case.log`
- `scripts/check_progress_rv64_gcc_c_torture_backend.sh`
- `src/backend/bir/lir_to_bir.cpp`
- `src/backend/bir/lir_to_bir/calling.cpp`
- `src/backend/bir/lir_to_bir/memory/`
- `tests/backend/bir/`

## Current Targets

- Current evidence anchor: stable 2026-07-02 reset-main RV64 gcc_torture
  backend-object scans with `1467` total, `349` passed, and `1118` failed.
- Candidate diagnostic family: row logs containing `semantic lir_to_bir`.
- Candidate producer topics: local-memory facts, call metadata, aggregate
  facts, publication gaps, and malformed or intentionally rejected inputs.
- Candidate non-BIR routes: prepared contract gaps, RV64/MIR object lowering,
  runtime mismatch, test infrastructure, F128 quarantine, or evidence gaps.

## Non-Goals

- Do not implement RV64 object lowering in this runbook.
- Do not bypass BIR producer defects from prepared-module or MIR consumers.
- Do not infer missing facts downstream when the BIR producer did not publish
  them.
- Do not use historical branch counts as current evidence.
- Do not weaken semantic admission checks, gcc_torture expectations,
  unsupported markers, allowlists, or runtime comparison behavior.
- Do not mix F128 or long-double work into ordinary-C producer admission work.

## Working Model

- The active failure map explicitly says BIR semantic producer admission is
  high priority but lacks a verified current row count.
- The first useful packet is evidence reconstruction, not implementation.
- Current per-case logs are the source of truth for row membership.
- Follow-up ideas should be created only for coherent, high-frequency current
  producer families with clear first ownership and proof shape.

## Execution Rules

- Keep packet progress, extracted row paths, commands, and proof notes in
  `todo.md`.
- Use the 2026-07-02 current scan artifacts unless the supervisor explicitly
  requests a fresh scan.
- If a fresh scan is requested, preserve its timestamped top-level log and
  update the current-log pointer only as delegated.
- Treat `semantic lir_to_bir` string matches as candidate rows; inspect each
  representative log before assigning owner.
- Prefer row tables under `build/agent_state/` or focused docs under
  `docs/rv64_gcc_torture_post_contract/` for evidence artifacts; do not
  encode row inventories only in chat.
- Any code-changing packet needs fresh build proof plus the exact focused
  command delegated by the supervisor.
- If a packet only reconstructs evidence or writes lifecycle/docs artifacts,
  record that no compile proof was required.

## Steps

### Step 1: Reconstruct Current BIR Admission Rows

Goal: produce a traceable current row set for `semantic lir_to_bir` admission
failures or prove that no verified current row set exists.

Actions:

- Read the current scan pointer, summary TSV, failed-case list, and relevant
  per-case logs.
- Extract candidate rows whose logs contain `semantic lir_to_bir` or related
  BIR lowering-pipeline admission notes.
- For each candidate, record the case path, case log path, visible diagnostic
  text, function name when present, and whether the log has enough evidence to
  identify a first BIR producer topic.
- Separate current evidence from historical support; do not reuse old branch
  counts as row counts.
- Store the reconstructed row table or evidence notes in an auditable artifact
  and summarize the artifact path in `todo.md`.

Completion check:

- `todo.md` names the current evidence artifact, current row count or explicit
  no-verified-row-set result, and representative logs inspected.

### Step 2: Classify Producer Families

Goal: classify verified rows by first owner and BIR producer topic without
turning downstream gaps into producer claims.

Actions:

- Split verified BIR admission rows into local-memory, call metadata,
  aggregate facts, publication gaps, malformed/intentionally rejected inputs,
  and unknown/evidence-gap groups.
- For each group, identify the first missing fact or malformed semantic input
  that prevents admission.
- Route rows that are not BIR producer failures back to prepared contract,
  RV64/MIR object lowering, runtime mismatch, test infrastructure, F128
  quarantine, or evidence-gap owners.
- Compare classifications against focused BIR tests only as support, not as a
  substitute for current row evidence.

Completion check:

- `todo.md` summarizes group counts, first owners, rejected non-BIR routes,
  and any rows left as evidence gaps.

### Step 3: Generate Follow-Up Routing

Goal: convert high-frequency current producer families into coherent follow-up
ideas or explicitly leave them unactivated when evidence is insufficient.

Actions:

- For each coherent high-frequency BIR producer family, draft the intended
  owner, proof shape, and reviewer reject signals.
- Keep local-memory, call metadata, aggregate facts, and publication gaps
  separate unless row evidence proves they share one producer boundary.
- Route non-BIR families to existing open ideas when they fit, or request a
  separate lifecycle action for new ideas.
- Do not edit source ideas during routine execution unless the supervisor
  delegates lifecycle ownership.

Completion check:

- `todo.md` lists follow-up candidates, rejected or deferred candidates, and
  the exact lifecycle action needed next, if any.

### Step 4: Prove The Reconstruction Outcome

Goal: make the evidence reconstruction auditable and ready for supervisor
acceptance or lifecycle follow-up.

Actions:

- Verify that the row counts reconcile with the selected current scan
  artifacts.
- Confirm no implementation files, tests, expectations, unsupported markers,
  allowlists, or runtime comparison behavior were weakened.
- If follow-up ideas are needed, hand the supervisor precise plan-owner
  lifecycle inputs rather than silently expanding this runbook.
- If no verified current row set exists, record the reason and the next
  evidence action instead of inventing an implementation queue.

Completion check:

- `todo.md` states whether the source idea is satisfied, needs follow-up idea
  creation, needs a fresh scan, or remains blocked by missing evidence.
