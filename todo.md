Status: Active
Source Idea Path: ideas/open/187_bir_memory_provenance_global_handle_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory memory/provenance global-handle surfaces

# Current Packet

## Just Finished

Lifecycle close/switch completed: idea 186 was closed after close-time backend
regression guard passed, and idea 187 is now active at Step 1.

## Suggested Next

Execute Step 1 from `plan.md`: inventory memory/provenance global-handle
surfaces under `src/backend/bir/lir_to_bir/memory`, classify route-local versus
global symbol identity uses, and record the selected Step 2 hardening target.

## Watchouts

- The source idea's dependency metadata still names the old `ideas/open/185...`
  and `ideas/open/186...` paths; those dependencies are now closed and the
  active runbook points readers at the closed archive paths.
- Do not treat every string-keyed memory/provenance map as semantic global
  identity. Local slots, local SSA names, temporaries, and explicit raw-import
  compatibility may remain route-local strings.
- Idea 187 is still a prerequisite before the final freeze gate in idea 188.

## Proof

Close-time regression guard for idea 186 passed using existing canonical
backend logs:

- command: `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`
- before: 109 passed, 0 failed
- after: 109 passed, 0 failed
