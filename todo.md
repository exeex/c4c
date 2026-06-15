Status: Active
Source Idea Path: ideas/open/269_phase_f4_prepared_bir_module_metadata_proof_gate.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Decide Gate Status

# Current Packet

## Just Finished

Step 4, `Decide Gate Status`, completed the analysis-only gate decision for
`PreparedBirModule::invariants`,
`PreparedBirModule::completed_phases`, and `PreparedBirModule::notes`.

Gate decision: blocked.

Step 2 proved that the current public printer paths preserve valid prepared
metadata text for all three fields. Step 2 did not prove metadata-derived
status or debug fail-closed authority for any field; those rows remain retained
compatibility surfaces, not demotion authority. Step 3 proved that all required
invalid, mismatched, missing, and absent fail-closed rows are blocked today, and
that direct payload reads are observable but not fail-closed-proven. Therefore
no later private-pass-context, demotion, wrapper, adapter, migration, metadata
authority movement, or broad prepared-retirement packet is authorized by this
runbook.

Row-category gate map:

| Row category | `invariants` | `completed_phases` | `notes` | Gate result |
| --- | --- | --- | --- | --- |
| Printer preservation | `satisfied`: valid invariant rows are retained through prepared printer and dump surfaces. | `satisfied`: valid phase rows are retained through prepared printer and dump surfaces. | `satisfied`: valid note rows are retained through prepared printer and focused dump surfaces. | Printer rows remain public compatibility authority and must not be weakened. |
| Status preservation | `blocked`: AArch64 handoff/status checks target arch and ABI, not invariant metadata. | `blocked`: AArch64 handoff/status checks target arch and ABI, not completed phase metadata. | `blocked`: live status/failure text consumes lowering notes separately and does not prove prepared-note authority. | Requires a separate implementation/test-proof idea before status can be metadata authority. |
| Debug preservation | `observable-unproven`: route/debug paths may carry the prepared aggregate, but no invariant-specific debug contract was proven. | `observable-unproven`: route/debug paths may carry the prepared aggregate, but no phase-specific debug contract was proven. | `observable-unproven`: route/debug/focused dump paths expose note text, but do not prove fail-closed debug authority. | Debug rows are retained compatibility surfaces only. |
| Invalid metadata | `blocked`: no validator rejects invalid or semantically impossible invariant evidence. | `blocked`: no allowlist/order/duplicate validator rejects invalid phase strings. | `blocked`: no note schema validator rejects malformed or misleading prepared notes. | Needs a separate implementation/test-proof idea with field-specific negative rows. |
| Mismatched metadata | `blocked`: no live row cross-checks invariant claims against BIR content before lowering. | `blocked`: no live row cross-checks phase claims against prepared facts before lowering. | `blocked`: no live row rejects prepared-note claims that do not match prepared facts or that conflate lowering notes. | Needs semantic validation work in a separate lifecycle idea. |
| Missing metadata | `blocked`: missing invariant metadata is printer omission, not handoff rejection. | `blocked`: missing phase metadata is printer omission, not handoff rejection. | `blocked`: missing prepared notes are not proven required or rejected. | Missing rows block any metadata authority movement. |
| Direct payload read | `observable-unproven`: printer/tests directly read public invariant payloads. | `observable-unproven`: printer/tests directly read public phase payloads. | `observable-unproven`: printer and focused dump filters directly read public note payloads. | Direct reads stay public compatibility authority until proven to be mirrors with fail-closed behavior. |
| Absent metadata | `blocked`: absent invariant metadata is represented by an empty/unpopulated vector and is not rejected. | `blocked`: absent phase metadata is represented by an empty/unpopulated vector and is not rejected. | `blocked`: absent prepared notes are omitted and not proven optional or rejected. | Needs field-specific negative proof before any later proposal. |

Retained public prepared compatibility authority:

- `PreparedBirModule::invariants`: the public vector field, producer append
  sites for `NoTargetFacingI1` and `NoPhiNodes`, prepared printer invariant
  block, `--dump-prepared-bir`, focused prepared dumps, direct helper/oracle
  reads, AArch64 prepared handoff diagnostics/status surfaces that receive the
  aggregate, x86 route debug/status aggregate plumbing, and exact
  target-output/unsupported behavior that depends on the prepared route.
- `PreparedBirModule::completed_phases`: the public vector field, producer
  append sites for `legalize`, `stack_layout`, `liveness`, `out_of_ssa`, and
  `regalloc`, prepared printer `completed_phases:` line, CLI and focused
  prepared dumps, AArch64 handoff diagnostics/status surfaces that receive the
  aggregate, x86 route debug/status aggregate plumbing, and exact
  target-output/unsupported behavior that depends on the prepared route.
- `PreparedBirModule::notes`: the public vector field, producer append sites
  across prepare passes, prepared printer `notes:` block, focused prepared dump
  phase/message filtering, debug/status aggregate plumbing, and the preserved
  distinction between prepared notes and `BirLoweringResult::notes` in lowering
  failure/status messages.

Later-work decision:

- Private-pass-context work is blocked for these three metadata fields because
  the gate has unsatisfied fail-closed rows.
- Demotion, deletion, privatization, accessor wrapping, adapter migration, or
  metadata-authority movement is blocked in this lifecycle state.
- A later proposal is eligible only as a separate lifecycle idea and only if it
  starts from the retained compatibility surfaces above, preserves existing
  printer/status/debug/helper/oracle/fallback/wrapper/exact-output/unsupported
  behavior, and adds semantic negative proof for invalid, mismatched, missing,
  direct-payload-read, and absent rows for all three fields.
- Metadata proof for these fields would not imply whole
  `PreparedBirModule` retirement. Broad prepared aggregate retirement remains
  out of scope and would need its own source idea, compatibility inventory, and
  proof gate.

Minimum conditions for any future proposal:

- Define field-specific validators or equivalent semantic gates for
  `invariants`, `completed_phases`, and `notes`.
- Add negative tests that reject invalid, mismatched, missing, and absent
  metadata for all three fields without relying on named-fixture shortcuts,
  undefined enum casts, docs-only assertions, expectation weakening, or
  status/debug relabeling.
- Prove every direct payload read is either a retained compatibility mirror or
  participates in fail-closed validation.
- Preserve public printer text, dump/focused-dump behavior, route debug/status
  behavior, helper/oracle contracts, lowering-note distinction, exact target
  outputs, unsupported behavior, and baseline expectations.
- Keep any implementation or test-proof work in a separate lifecycle idea; this
  proof-gate runbook does not authorize code changes.

## Suggested Next

Execute Step 5 from `plan.md`: validate that this gate decision did not drift
into implementation, expectation weakening, metadata demotion, or whole
`PreparedBirModule` retirement. Supervisor/reviewer scrutiny is appropriate
before lifecycle closure because the gate result is blocked.

## Watchouts

- This active plan is proof-gate work only.
- Do not demote, delete, privatize, wrap, migrate, or implement authority
  movement for `PreparedBirModule` metadata.
- Do not weaken printer, status, debug, helper/oracle, fallback, wrapper,
  exact target-output, unsupported, or baseline behavior.
- Do not claim progress through expectation rewrites, helper renames,
  status/oracle relabeling, printer/debug formatting changes,
  classification-only notes, docs-only assertions, or named-fixture proof.
- AArch64 handoff currently validates target arch and ABI only; it does not
  validate `invariants` or `completed_phases`.
- x86 route debug receives the prepared aggregate but does not directly render
  or validate these three fields.
- Focused prepared dump note filtering is textual compatibility behavior, not
  proof that prepared notes fail closed.
- The Step 4 decision is blocked because every invalid, mismatched, missing,
  and absent row remains blocked for all three fields.
- Printer preservation rows are satisfied for valid metadata text, but
  satisfied printer rows do not prove status/debug fail-closed authority.
- Direct payload read rows are observable-unproven, not satisfied. Treat them
  as retained public compatibility authority.
- Metadata proof for `invariants`, `completed_phases`, and `notes` does not
  imply whole `PreparedBirModule` retirement.

## Proof

Analysis-only packet. Required proof:

- `cat todo.md`
- `git status --short`
- `git diff --check`

No build or CTest was required because no implementation files were changed.
No `test_after.log` update was made because the delegated proof for this
analysis-only packet explicitly required readback/status/diff-check only and
the packet forbade touching root proof logs.
