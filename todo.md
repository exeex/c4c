# Current Packet

Status: Active
Source Idea Path: ideas/open/803_bir_exceptional_control_allocation_and_frame_design_completion.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Define allocation facts and deterministic choice policy

## Just Finished

- Completed plan Step 4: selected versioned fail-closed complete promotion,
  defined deterministic pre-mutation checked resource planning and exact
  duplicate-edge accounting, required atomic candidate rollback, and aligned
  B5/P04 admission without inventing a retained partial-promotion form.

## Suggested Next

- Execute plan Step 5 and separate immutable E1 allocation cost facts from
  versioned deterministic E2 allocation/coalescing/eviction choices.

## Watchouts

- Documentation-only: do not modify code, tests, build files, scripts,
  generated artifacts, binaries, canonical regression logs, or unrelated
  lifecycle sources.
- Preserve Step 4's fail-closed complete-promotion decision. Partial promotion
  requires a future explicit B4/B5 schema change and is not an allocator or
  memory-pass fallback.

## Proof

- Documentation-only proof: `git diff --check`; Markdown-only changed-path
  audit; introduced relative-link validation; and focused audits for the
  selected/rejected policy rationale, versioned metrics, checked arithmetic,
  finite hard bounds, stable identity ordering/ties, exact-revision keys,
  pre-mutation planning, duplicate edge-occurrence accounting, complete
  realization, atomic rollback, B5 admission, and absence of mid-pass fallback.
  No build/runtime test applies; canonical logs were untouched.
