# Current Packet

Status: Active
Source Idea Path: ideas/open/803_bir_exceptional_control_allocation_and_frame_design_completion.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define non-local control-transfer safety

## Just Finished

- Completed plan Step 1: inventoried every affected owner and published one
  normative provisional placement/conflict matrix covering all six idea 803
  design areas without changing the A1-F3 spine or F1 boundary.

## Suggested Next

- Execute plan Step 2 and define non-local control-transfer safety across the
  now-fixed B4/B5 -> E1/E2/E3 -> E4 owner chain.

## Watchouts

- Documentation-only: do not modify code, tests, build files, scripts,
  generated artifacts, binaries, canonical regression logs, or unrelated
  lifecycle sources.
- Keep exceptional-boundary facts immutable and revision-keyed; they cannot
  become a second CFG authority or an allocator-only special case.
- The Step 1 matrix intentionally does not choose the Step 4 promotion policy
  or close the detailed contracts reserved for Steps 2-7.

## Proof

- Documentation-only proof: `git diff --check`; Markdown-only changed-path
  audit; affected-owner inventory; focused A1-F3/F1 checks; and validation of
  changed relative Markdown links. No build/runtime test applies, and canonical
  regression logs were not created or modified.
