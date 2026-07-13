Status: Active
Source Idea Path: ideas/open/730_post_legacy_bir_shell_bootstrap.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Restore CMake generation without legacy sources

# Current Packet

## Just Finished

- Parked idea 714 and activated the post-legacy BIR shell bootstrap route.

## Suggested Next

- Remove obsolete backend test targets blocking CMake generation, verify legacy
  paths stay outside build graphs, then configure and build to capture the next
  true seam.

## Watchouts

- Never compile or transplant `src/backend/legacy` code.
- Bootstrap only explicit empty BIR/MIR behavior; do not imply target codegen.
- Retain tests only for direct LIR-to-new-BIR and new-BIR-to-MIR contracts.

## Proof

- Not run; lifecycle-only plan switch.
