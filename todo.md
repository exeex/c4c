# Execution State

Status: Active
Source Idea Path: ideas/open/80_draft_replacement_x86_codegen_interfaces_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Materialize Manifest Coverage And Directory Skeleton
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Lifecycle switch on 2026-04-22: closed idea 79 and activated the stage-3
draft-generation runbook from idea 80.

## Suggested Next

Begin plan step 1 by materializing the exact stage-2 manifest under
`docs/backend/x86_codegen_rebuild/`, including the directory/index artifacts
and every required `.cpp.md` / `.hpp.md` path.

## Watchouts

- The stage-2 manifest is binding. Missing, renamed, merged, or dropped draft
  files are a contract violation unless stage 2 is repaired first.
- Prepared routes must stay consumers of canonical seams instead of growing a
  second lowering stack in the draft tree.
- The rebuild boundary still excludes the legacy `peephole/` subtree.

## Proof

Lifecycle-only switch on 2026-04-22. Close-time regression guard reused the
existing canonical logs with:
`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
