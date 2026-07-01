Status: Active
Source Idea Path: ideas/open/483_bir_semantic_local_memory_scalar_load_producer.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Focused Local-Load Representative Evidence

# Current Packet

## Just Finished

Repaired Step 1 lifecycle state after reviewer route review rejected the dirty
selection of `src/pr22098-1.c` `(*p#L1) != 1` as the first Step 2 target.

Reviewer input:

- `review/483_step1_local_load_route_review.md`

Corrected disposition:

- The dirty Step 1 audit usefully classified the five representative rows, but
  its accepted `pr22098-1.c` shape is not valid for idea 483's first ordinary
  scalar local-load packet.
- `pr22098-1.c` `*p#L1` depends on pointer-to-local-array-element provenance
  from `p#L1 = &<clit>#L3[(++a#L0)]`.
- Idea 483 explicitly excludes pointer/provenance-dependent loads from the
  ordinary scalar local-load route and requires those shapes to remain
  fail-closed unless a separate lifecycle split selects them.
- The sibling `(*((int*)b#L2))` integer-pointer round-trip remains rejected.

Repaired representative status:

| Case | Prior Step 1 classification | Corrected lifecycle disposition |
| --- | --- | --- |
| `src/20041124-1.c` | Aggregate return/global field plus complex member. | Rejected for 483 first packet. |
| `src/20071219-1.c` | Aggregate struct copy through global pointer with memset/call contamination. | Rejected for 483 first packet. |
| `src/991228-1.c` | Union type-pun and global pointer-cast index/object-representation boundary. | Rejected for 483 first packet. |
| `src/multi-ix.c` | Direct scalar array evidence exists, but latest failure is variadic/va_arg contaminated. | Not selected; needs a non-contaminated proof before it can drive 483. |
| `src/pr22098-1.c` | Previously accepted only with explicit pointer-to-local-element provenance. | Rejected for 483 first packet as pointer/provenance-dependent route drift. |

Current Step 1 status:

- No implementation-ready ordinary scalar local-object load representative is
  selected from the current five-row audit.
- Idea 483 remains active because the source intent is still valid for the
  79-row local-memory load bucket.
- The next Step 1 packet must find a non-pointer/provenance ordinary scalar
  local-load representative, or record an exact blocker if the bucket requires
  a separate pointer/provenance or other producer prerequisite before any 483
  implementation packet can proceed.

## Suggested Next

Continue Step 1 under idea 483 with a corrected representative search:

- scan additional rows from the 79-row local-memory load bucket for an
  ordinary scalar local-object load that does not require GEP,
  pointer/provenance, aggregate/member, byval/va_arg, volatile/atomic, complex,
  vector, F128, bootstrap, or target fallback inference;
- if none exists, record that exact blocker and route lifecycle to the proper
  producer prerequisite instead of broadening idea 483;
- if `pr22098-1.c` remains strategically important, split it into a separate
  pointer-to-local-element provenance source idea before using it as a contract
  driver.

## Watchouts

- Do not proceed to Step 2 with `pr22098-1.c` `*p#L1` as the selected scalar
  local-load target under idea 483.
- Do not repair missing BIR semantic facts in RV64/MIR.
- Do not infer producer facts from testcase names, value names, raw dump order,
  final homes, or target fallback behavior.
- Keep pointer/provenance-dependent loads fail-closed unless a separate source
  idea explicitly owns them.
- Do not touch `review/`, canonical logs, baseline files, implementation files,
  or tests.

## Proof

Lifecycle validation:

```sh
git diff --check
python3 scripts/plan_review_state.py show
```

Result: passed.
