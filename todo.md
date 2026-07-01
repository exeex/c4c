Status: Active
Source Idea Path: ideas/open/482_semantic_frame_slot_materialization_probe_decomposition.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish The Blocked Failure-Family Baseline

# Current Packet

## Just Finished

Completed Step 1 for idea 482 by establishing the blocked semantic frame-slot
materialization failure-family baseline from the existing 475 -> 481 route.

Concrete repeated symptoms:

| Link | Repeated symptom | First missing owner named there |
| --- | --- | --- |
| 475 | `%t23` identity, slot `#21`, frame object/layout, and branch relationship were present, but no source fact could be populated. | Semantic instruction-result frame-slot materialization/write records plus interval no-clobber facts. |
| 476 | Synthetic explicit interval records worked, but the real `%t23` row still lacked real event and interval facts. | Real semantic materialization/write event and event-to-consumer interval population. |
| 477 | Real semantic fact population still had `%t23` identity and destination only. | Authoritative semantic instruction-result frame-slot write/materialization event carrier. |
| 478 | The carrier/status API could represent explicit events, but real `%t23` had no event authority. | Real event-authority producer. |
| 479 | Event-authority population still had no durable source proving `%t23` materialization into slot `#21`. | Lower-level semantic instruction-result frame-slot write/materialization event producer. |
| 480 | Write-event production still lacked an explicit materialization point. | Explicit semantic result frame-slot materialization-point producer. |
| 481 | The materialization-point producer idea remains valid but is parked by idea 482. | Resume only after focused probes bind a smaller generic seam. |

Monolithic owner family:

- semantic result `%t23 = bir.ne i32 %t22, 0`;
- branch condition `%t23 compare=ne i32 %t22, 0`;
- value id `17`, final home slot `#21`, storage `slot#21+stack156`;
- stack object `#21`, offset `156`, size `4`, align `4`;
- rejected nearby movement `%t22 -> %t23`, `from_value_id=16`,
  `to_value_id=17`, `authority=none`;
- protected `%t22` select-result stack-destination boundary.

Seams that must not be conflated:

| Seam | Baseline status |
| --- | --- |
| Semantic instruction-result identity | Present for `%t23`; not sufficient by itself. |
| Frame-slot destination/layout | Present for slot `#21`; placement only. |
| Materialization point / producer authority | Missing; first true blocker for 481-style work. |
| Storage-only movement | `%t22 -> %t23 authority=none` must remain rejected. |
| Select-result stack destination | `%t22` is a protected separate owner. |
| Pointer/provenance rows | `%t1` / `%t7` are separate boundaries. |
| Unsupported-terminator rows | `%t2` / `%t8` are separate boundaries. |
| Path/no-clobber interval | Downstream of explicit event production. |
| Source-fact and branch-stack-load authority | Downstream of event plus interval/source facts. |
| RV64 branch-load emission | Downstream non-goal. |

Accepted baseline facts:

- `%t23` semantic identity and branch-condition use are durable enough to form
  a focused semantic-identity probe.
- Slot `#21` destination facts are durable enough to form a focused
  destination/layout probe.
- Existing dispositions consistently reject `authority=none` storage movement
  as semantic materialization evidence.
- Explicit synthetic event carriers are representable when supplied as
  explicit inputs.

Rejected baseline facts:

- Raw BIR adjacency, branch condition shape, final homes, storage plans, stack
  object ids, offsets, value names, function names, testcase shape, and dump
  order are not materialization authority.
- `%t22 -> %t23` with `authority=none` is storage-only movement, has the wrong
  semantic source, and cannot populate `%t23` materialization.
- Copying the full `930930-1` shape into a new focused case would duplicate
  the monolith instead of isolating one seam.
- Downstream interval/source-fact/branch authority/RV64 work remains blocked
  until an explicit materialization-point seam is proven or replaced by a
  smaller focused probe.

Artifact:

- `build/agent_state/482_step1_blocked_family_baseline/baseline.md`

## Suggested Next

Execute Step 2 by splitting the monolithic probe into focused cases. The first
probe-splitting target should be a scalar compare result forced to a frame-slot
destination, isolating semantic identity plus final slot facts while excluding
select-result stack destinations, storage-only moves, interval/source-fact
population, branch-stack-load authority, and RV64 consumption.

## Watchouts

- Do not implement materialization-point production in Step 2.
- Do not copy the full `930930-1` graph into `tests/backend/case/`.
- Do not rely on raw BIR, final homes, storage, stack objects, offsets, names,
  testcase shape, or dump order as authority.
- Keep storage-only move rejection, select-result stack-destination, and
  synthetic explicit materialization-point probes as separate candidate
  families unless Step 2 proves a different split is cleaner.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
