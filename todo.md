Status: Active
Source Idea Path: ideas/open/225_aarch64_module_phoenix_extract_legacy_evidence.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Teardown Accepted Legacy CPP

# Current Packet

## Just Finished

Step 4: Teardown Accepted Legacy CPP completed for the accepted AArch64 module
legacy implementation. Confirmed `module.cpp.md`, `module.hpp.md`, and
`module.md` exist, then deleted
`src/backend/mir/aarch64/module/module.cpp` with the phoenix cleanup script.
`src/backend/mir/aarch64/module/module.hpp` remains in place.

## Suggested Next

Proceed with Step 5 close-readiness over the extracted artifact set and the
teardown state before activating any later phoenix stage.

## Watchouts

- Treat deleted `module.cpp` as legacy evidence, not the design to restore or
  patch.
- Prepared BIR should lower directly to MIR machine nodes in the rebuilt
  route.
- Stage 1 still owns evidence extraction and teardown only; replacement layout
  and implementation belong to later phoenix stages.
- `module.md` now points at both extraction companions and should remain an
  evidence index, not an architecture draft.
- Do not edit `module.hpp`, markdown evidence files, CMake files, or
  replacement implementation files as part of this teardown slice.

## Proof

Ran the supervisor-selected teardown proof by first confirming
`module.cpp.md`, `module.hpp.md`, and `module.md` existed, then running exactly:
`python .codex/skills/phoenix-rebuild/scripts/delete_legacy_cpp.py 'src/backend/mir/aarch64/module/module.cpp'`.
Wrote `test_after.log` with evidence that the markdown files and `module.hpp`
are present and `src/backend/mir/aarch64/module/module.cpp` is absent. No build
was run because this packet was delegated as teardown-only phoenix evidence.
