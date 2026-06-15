# 274 Phase F5 remaining prepared boundary proof backlog

## Goal

Preserve the remaining prepared/BIR boundary proof work after ideas 270 through
273, so the next session can resume without relying on chat context.

This draft is a context handoff and backlog. It should not be opened as a broad
implementation plan. When work resumes, promote one bounded proof packet at a
time from this backlog into `ideas/open/`.

## Current State

The latest completed work is:

- `ideas/closed/270_phase_f5_memory_accesses_prepared_only_same_consumer_fail_closed_proof.md`
- `ideas/closed/271_phase_f5_x86_riscv_memory_accesses_public_consumer_fixture_support.md`
- `ideas/closed/272_phase_f5_riscv_memory_accesses_prepared_only_fail_closed_proof.md`
- `ideas/closed/273_phase_f5_riscv_memory_accesses_byte_offset_drift_fail_closed_proof.md`

Current lifecycle state at handoff:

- no active `plan.md`;
- no active `todo.md`;
- no open ideas besides `.keep`;
- accepted baseline is green at 3428/3428;
- no `test_baseline.new.log` candidate is pending.

## What Has Been Reduced

### memory_accesses prepared-only row

Idea 272 proved one RISC-V same-consumer prepared-only `LoadLocal` row:

- real backend consumer directly reads `PreparedFunctionLookups::memory_accesses`;
- internally coherent prepared memory row without matching Route 3 / Route 5
  authority fails closed;
- observed fail-closed status is `UnsupportedSourceHome`;
- positive path remains byte-stable at `lw a1, 12(s2)`;
- this reduces only the selected RISC-V same-consumer prepared-only PO-family
  blocker from idea 265.

### memory_accesses byte-offset drift row

Idea 273 proved one RISC-V same-consumer byte-offset drift row:

- prepared/public `memory_accesses` row remains present at byte offset `12`;
- Route 3 memory/source authority is drifted to byte offset `16`;
- the real RISC-V consumer rejects semantic agreement;
- fail-closed evidence includes `UnsupportedSourceHome`, empty instruction
  text, `MemorySource`, `route5_edge_source_agrees=false`, and
  `route3_source_memory_agrees=false`;
- positive path remains byte-stable at `lw a1, 12(s2)`;
- this reduces only the selected RISC-V same-consumer BO-family blocker from
  idea 265.

## Remaining Work

### 1. memory_accesses stale-publication proof

Status: next recommended packet.

Why:

- prepared-only and byte-offset drift each now have one RISC-V same-consumer
  proof row;
- idea 265 still records stale-publication blockers;
- stale-publication is the next natural proof family before returning to a
  broader gate.

Needed shape:

- one same-consumer row;
- preferably RISC-V, reusing the dynamic stack-source `LoadLocal` fixture if
  the stale row can be expressed through supported construction;
- prove old/stale prepared memory facts do not become Route 3 / Route 5
  semantic agreement;
- preserve positive output and compatibility rows.

Reject if:

- it requires hand-built stale prepared state that normal construction would
  reject;
- it proves helper-only behavior while claiming backend same-consumer proof;
- it weakens status/output/fallback/helper/oracle/baseline behavior.

Suggested next idea name:

`274_phase_f5_riscv_memory_accesses_stale_publication_fail_closed_proof.md`

If using this draft number for the actual open packet, move/rename this backlog
first or choose the next number for the open packet.

### 2. memory_accesses cross-publication mismatch proof

Status: do after stale-publication unless the stale packet discovers this is
the better supported row.

Remaining blocker from idea 265:

- no combined memory-source target proof exhausts internally consistent but
  wrong prepared owners;
- duplicate/conflicting Route 5 memory-source rows;
- wrong-edge or wrong-destination memory-source rows;
- cross-target status/fallback/output matrices.

Needed shape:

- one real target consumer;
- one prepared memory/source row whose owner/publication does not match the
  Route 3 / Route 5 semantic authority;
- fail closed without accepting the wrong publication as agreement;
- preserve exact target output and compatibility surfaces.

### 3. memory_accesses exhaustive byte-offset coverage

Status: not done.

Idea 273 only proves one selected RISC-V same-consumer row. It does not clear:

- every synthetic prepared offset drift row;
- all public lookup consumers;
- all target operand surfaces;
- all wrapper/exact-output contracts.

This likely needs either:

- another one-row proof packet for a different consumer/surface; or
- a later proof-map gate once stale-publication and cross-publication rows have
  more evidence.

### 4. x86 memory_accesses public-field parity

Status: still missing.

Known state:

- x86 has adjacent Route 3 / Route 5 agreement support;
- x86 paths mostly consume prepared edge-publication source-memory and
  addressing records, not the public `PreparedFunctionLookups::memory_accesses`
  field;
- 270 found no x86 or riscv public-field consumer at the time;
- 271 created a RISC-V public-field consumer fixture, not an x86 one.

Do not claim x86 parity from the current RISC-V proofs. A future x86 packet
needs a real public-field consumer or explicit fail-closed non-applicability.

### 5. edge_publications proof families

Still blocked from idea 266:

- duplicate Route 5 records on one natural edge;
- prepared-only publication rows;
- Route 5-only publication rows;
- wrong-edge publication rows.

No demotion, deletion, private accessor, wrapper, adapter migration, or broad
prepared retirement is safe from the current proof maps.

### 6. edge_publication_source_producers

Still blocked from idea 267:

- no complete concrete x86 consumer boundary for
  `PreparedFunctionLookups::edge_publication_source_producers`;
- Route 5 edge-publication move gate is partial copied-publication support for
  `LoadLocal`;
- non-`LoadLocal` compatibility fallback remains the key blocker.

Do not open implementation work until a real x86 consumer boundary is named.

### 7. metadata and liveness

Still not demotion-ready:

- `PreparedBirModule::invariants`;
- `PreparedBirModule::completed_phases`;
- `PreparedBirModule::notes`;
- `PreparedBirModule::liveness`.

From ideas 268 and 269:

- metadata fields need printer/status/debug preservation plus invalid,
  mismatched, missing, direct-payload-read, and absent metadata proof;
- liveness has no exact identity-only reader and no independent semantic
  liveness fact outside public `PreparedBirModule::liveness`.

Do not open private-pass-context work for these yet.

### 8. prepared aggregate retirement

Still blocked:

- no whole `PreparedFunctionLookups` retirement;
- no whole `PreparedBirModule` retirement;
- no draft 155 opening;
- no broad public prepared API hiding.

`PreparedBirModule::route` is already private pass context, but that does not
authorize broader aggregate retirement.

## Recommended Next Action

Open exactly one bounded proof packet:

`Phase F5 RISC-V memory_accesses stale-publication fail-closed proof`

The packet should:

- start by identifying whether the supported RISC-V dynamic stack-source
  `LoadLocal` fixture can express a stale-publication row without synthetic
  stale state;
- name the prepared row, stale Route 3 / Route 5 authority relation, expected
  fail-closed status, and stable output;
- prove the stale row fails closed, or close as a blocker and create a narrower
  fixture-support idea.

## General Guardrails

- Do not accept helper-only proof as backend same-consumer proof.
- Do not hand-build stale prepared state that a normal route would reject.
- Do not weaken expected output, unsupported status, helper/oracle status,
  fallback behavior, route-debug output, prepared-printer output, wrapper
  output, exact target output, or baseline behavior.
- Do not claim demotion, deletion, privatization, accessor wrapping, or draft
  155 readiness from one proof row.
- Keep target policy out of BIR: stack, storage, addressing, source-home,
  register, layout, wrapper, formatting, emission, instruction spelling, and
  exact output remain target-owned.
