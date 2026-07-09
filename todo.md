Status: Active
Source Idea Path: ideas/open/635_prepared_branch_stack_clobber_safety_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Prepared Branch Clobber-Safety Evidence

# Current Packet

## Just Finished

Step 1 refreshed the prepared branch stack-load clobber-safety evidence for the
delegated seven-row allowlist. The allowlist is
`build/agent_state/635_step1_branch_clobber_safety.allowlist`; the compact row
classification is
`build/agent_state/635_step1_branch_clobber_safety.classification.tsv`; focused
prepared dumps live at `build/agent_state/635_step1_*.prepared.txt`.

All seven rows are in-scope selected-freshness / missing-clobber-safety rows:

| Row | First owner and use point | Selected candidate identity | Visible clobber-safety facts |
| --- | --- | --- | --- |
| `src/20001017-1.c` | `unsupported_branch_stack_load_authority`; `bug` `entry`, branch terminator index `1`, `lhs` `%p.C` | `source_freshness_status=selected`, `source_freshness_candidates=1`, `branch_stack_slot`, `%p.C` value id `11`, `branch_terminator_ordering`, ref block `0`, ref inst `1` | `policy=load_from_stack_slot`, `pointer_status=proven`, `status=missing_stack_clobber_safety`, slot `#13`, object `#12`, stack offset `56` |
| `src/loop-2e.c` | `unsupported_branch_stack_load_authority`; `main` `block_6`, branch terminator index `14`, `rhs` `%t23` | `selected`, one `branch_stack_slot` candidate, `%t23` value id `26`, `branch_terminator_ordering`, ref block `3`, ref inst `14` | `load_from_stack_slot`, `pointer_status=proven`, `missing_stack_clobber_safety`, slot `#46`, object `#51`, stack offset `336` |
| `src/pr39100.c` | `unsupported_branch_stack_load_authority`; `foo` `block_1`, branch terminator index `2`, `lhs` `%t0` | `selected`, one `branch_stack_slot` candidate, `%t0` value id `4`, `branch_terminator_ordering`, ref block `1`, ref inst `2` | `load_from_stack_slot`, `pointer_status=proven`, `missing_stack_clobber_safety`, slot `#41`, object `#42`, stack offset `152` |
| `src/20000314-3.c` | `unsupported_branch_stack_load_authority`; `attr_rtx` `entry`, branch terminator index `1`, `rhs` `@arg0` | `selected`, one `branch_stack_slot` candidate, `@arg0` value id `3`, `branch_terminator_ordering`, ref block `0`, ref inst `1` | `load_from_stack_slot`, `pointer_status=proven`, `missing_stack_clobber_safety`, slot `#0`, object `#1`, stack offset `0` |
| `src/20140828-1.c` | `unsupported_branch_stack_load_authority`; `main` `entry`, branch terminator index `3`, `rhs` `%t6` | `selected`, one `branch_stack_slot` candidate, `%t6` value id `18`, `branch_terminator_ordering`, ref block `0`, ref inst `3` | `load_from_stack_slot`, `pointer_status=proven`, `missing_stack_clobber_safety`, slot `#16`, object `#17`, stack offset `8` |
| `src/20080519-1.c` | `unsupported_branch_stack_load_authority`; `regrename_optimize` `for.cond.4`, branch terminator index `3`, `lhs` `%t8` | `selected`, one `branch_stack_slot` candidate, `%t8` value id `13`, `branch_terminator_ordering`, ref block `1`, ref inst `3` | `load_from_stack_slot`, `pointer_status=proven`, `missing_stack_clobber_safety`, slot `#32`, object `#32`, stack offset `112` |
| `src/20050125-1.c` | `unsupported_branch_stack_load_authority`; `bracket_empty` `entry`, branch terminator index `3`, `rhs` `%t3` | `selected`, one `branch_stack_slot` candidate, `%t3` value id `4`, `branch_terminator_ordering`, ref block `0`, ref inst `3` | `load_from_stack_slot`, `pointer_status=proven`, `missing_stack_clobber_safety`, slot `#13`, object `#13`, stack offset `40` |

No row in this packet currently has a no-candidate source-freshness first owner,
select-publication first owner, generic terminator first owner, move-bundle
first owner, unrelated ABI/runtime first owner, or accounting-only outcome.
There are ancillary non-first-owner records in the prepared dumps, such as
`pr39100.c` line `799` and `20080519-1.c` lines `690` and `693`, but the
runner-selected first owner for each delegated row is the missing
branch-stack-load clobber-safety authority above.

## Suggested Next

Step 2 packet boundary: repair the semantic clobber-safety authority for
selected prepared branch stack-load pointer operands when the selected
`branch_stack_slot` freshness fact exactly matches the branch terminator use and
the pointer operand is proven. Keep the slice general over role/function/block:
it should make the authority status available only through a real
clobber-safety proof, not through named-source matching, expectation rewrites,
or final-assembly inference.

## Watchouts

The current failure family is narrower than branch freshness publication:
freshness is already selected for all seven first owners. Do not spend Step 2 on
no-candidate freshness rows, select-publication gaps, generic terminator gaps,
move bundles, ABI/runtime behavior, accounting, unsupported-marker changes,
allowlist edits, stack-home inference, final assembly inference, stack-offset
inference, or named-source shortcuts.

Prepared dumps show no accepted clobber-safe proof field for the first-owner
pointer rows; they show `pointer_status=proven` plus
`status=missing_stack_clobber_safety`. That is the semantic boundary to repair.

## Proof

Ran the delegated command:

`cmake --build --preset default && ALLOWLIST=build/agent_state/635_step1_branch_clobber_safety.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/635_step1_branch_clobber_safety.log 2>&1`

Result: build was up to date; progress check returned nonzero with
`total=7 passed=0 failed=7`. The nonzero result is classification evidence, not
an executor blocker: every failure is the expected Step 1 first owner,
`unsupported_branch_stack_load_authority` with selected freshness and
`missing_stack_clobber_safety`.

Proof/log paths:

- `build/agent_state/635_step1_branch_clobber_safety.log`
- `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`
- `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`
