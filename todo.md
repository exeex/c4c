# Current Packet

Status: Active
Source Idea Path: ideas/open/803_bir_exceptional_control_allocation_and_frame_design_completion.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Bound B4 promotion and phi growth

## Just Finished

- Completed plan Step 3: made B3 the sole owner of explicit optional
  fallthrough and exact asm-goto label occurrences, defined B4's single
  exact-revision instruction-point snapshot and edge-qualified visibility,
  preserved duplicate occurrence identity through phi construction and D5,
  and traced one complete scenario.

## Suggested Next

- Execute plan Step 4 and define a versioned, deterministic B4 promotion and
  phi-growth resource plan without permitting mid-pass fallback.

## Watchouts

- Documentation-only: do not modify code, tests, build files, scripts,
  generated artifacts, binaries, canonical regression logs, or unrelated
  lifecycle sources.
- Preserve Step 3's exact occurrence identity and instruction-point visibility
  when bounding promotion; resource policy must not weaken SSA correctness or
  collapse duplicate phi inputs.

## Proof

- Documentation-only proof: `git diff --check`; Markdown-only changed-path
  audit; introduced relative-link validation; and focused audits for
  zero/one/multiple goto targets, explicit fallthrough presence/absence,
  outputs/clobbers, duplicate successor occurrences, exact revision keys,
  critical-edge normalization, exact phi/D5 consumption, leakage rejection,
  invalidation, and exclusion of rendered/name identity. No build/runtime test
  applies; canonical logs were untouched.
