Status: Active
Source Idea Path: ideas/open/225_aarch64_module_phoenix_extract_legacy_evidence.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Write The Module Directory Index

# Current Packet

## Just Finished

Step 3: Write The Module Directory Index completed for the extracted AArch64
module scope. Created
`src/backend/mir/aarch64/module/module.md` as the directory-level evidence
index over `module.cpp.md` and `module.hpp.md`, summarizing the current module
emitter responsibilities, public module surface, adjacent API/codegen/prepared
BIR relationships, and the core lowering, optional fast path, legacy
compatibility, and overfit-to-reject classifications.

## Suggested Next

Proceed with the next stage-1 packet that checks stage-1 close readiness over
the per-file extraction artifacts and the directory-level `module.md` index.

## Watchouts

- Treat `module.cpp` as legacy evidence, not the design to patch.
- Prepared BIR should lower directly to MIR machine nodes in the rebuilt
  route.
- Stage 1 extracts evidence only; replacement layout belongs to stage 2.
- `module.md` now points at both extraction companions and should remain an
  evidence index, not an architecture draft.
- Do not broaden the scope beyond `module.cpp`, `module.hpp`, and their
  markdown evidence without lifecycle repair.

## Proof

Ran the supervisor-selected directory-index proof:
`rg -n 'module\.(cpp|hpp)\.md|core lowering|legacy compatibility|overfit' src/backend/mir/aarch64/module/module.md | tee test_after.log`.
Result: proof output references both `module.cpp.md` and `module.hpp.md` and
matches the classification labels `core lowering`, `legacy compatibility`, and
`overfit`. Proof log: `test_after.log`.
