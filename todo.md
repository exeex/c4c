Status: Active
Source Idea Path: ideas/open/616_select_publication_source_wiring.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Select Publication Residuals

# Current Packet

## Just Finished

Step 1, `Refresh Select Publication Residuals`, refreshed current diagnostics
from existing artifacts without implementation changes.

- Failure-map context still names `7` select publication stack-offset source
  wiring rows and `4` select publication move-bundle wiring rows.
- Artifact source:
  `build/agent_state/610_step3_move_bundle_residual_probe/summary.tsv` and
  per-case stderr logs.
- Select-publication rows observed: `13` total. In-scope stack-offset rows:
  `7`; source-home move-bundle rows: `4`; separate large-immediate rows: `2`.
- Stack-offset owner family: `unsupported_source_stack_offset` with
  `select_publication_evidence=yes`, `intent_status=available`, explicit
  stack-offset source facts, and publication source home `stack_slot`.
  Representative rows: `src/20000706-1.c`, `src/20000706-2.c`,
  `src/20000717-5.c`, `src/20071213-1.c`, `src/20120427-1.c`,
  `src/20120427-2.c`, `src/991216-1.c`.
- Move-bundle source-home family: `intent_status_unsupported_source_home`
  with `select_publication_evidence=yes`, but no explicit source offset or
  materializable source freshness in the intent. Representative rows:
  `src/pr45034.c`, `src/pr53160.c`, `src/pr58726.c`, `src/pr59221.c`.
- Large-immediate select-publication family: `2` rows
  (`src/pr29695-1.c`, `src/pr29695-2.c`) with `intent_status=available`, but
  owner is immediate range materialization, not this stack-offset/source-home
  packet.
- `src/921124-1.c`: current owner is `unsupported_terminator_fragment`; it is
  not currently a select-publication row, branch stack-source row, or
  in-scope move-bundle row.
- `src/920710-1.c`: current owner is `unsupported_terminator_fragment`; it is
  not currently a select-publication row, branch stack-source row, or
  in-scope move-bundle row.

## Suggested Next

Execute one bounded Step 2 consumer packet for the `7`
`unsupported_source_stack_offset` select-publication rows with
`intent_status=available`, preserving fail-closed behavior for the `4`
`intent_status_unsupported_source_home` move-bundle rows.

## Watchouts

- Do not treat alias evidence or destination legality as source freshness.
- Do not implement a named-case shortcut for a single stack-offset row.
- Keep branch stack-source, terminator, destination fan-in, runtime,
  expectation, unsupported-marker, allowlist, timeout, and accounting changes
  out of this idea.
- The `4` move-bundle source-home rows have publication evidence but not
  explicit source freshness in the current diagnostics; do not consume them as
  Step 2 positives unless a later probe proves the missing authority.
- `src/921124-1.c` and `src/920710-1.c` should remain classified under the
  terminator owner until terminator diagnostics expose a different first
  blocker.

## Proof

Diagnostics-only packet. Reused existing artifacts under `build/agent_state`
and `build/rv64_gcc_c_torture_backend`; no fresh proof command was run and no
`test_after.log` was created or modified.
