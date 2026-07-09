# Follow-Up Implementation Queue

Source idea: `ideas/open/618_runtime_mismatch_ownership_investigation.md`

This file answers the Step 4 follow-up question for the accepted July 9 RV64
gcc torture backend runtime evidence. The current baseline has `217` runtime
symptom rows:

- `110` abort or assertion rows.
- `102` segfault rows.
- `0` wrong-output rows.
- `5` timeout rows.

The implementation queue below is owner-first, not symptom-first. A runtime
exit status is not enough evidence to open a generic runtime-support fix.
Generic abort, segfault, and timeout rows need owner-specific evidence passes
or prerequisite reruns before implementation ownership is credible.

## Recommended Queue

| Priority | Owner | Work item | Proof surface | Initial rows |
| ---: | --- | --- | --- | --- |
| 1 | call lowering | External call relocation rerun for the dynamic-loader assertion | Focused object/backend rerun proving the generated external call relocation no longer trips `ld.so`, then RV64 torture rerun of the row | `src/990106-1.c` |
| 2 | ABI | Argument, return, and varargs runtime evidence lane | Targeted reruns that capture failed source predicate or qemu state, plus focused ABI backend tests before any fix | `src/va-arg-20.c`, `src/vprintf-1.c`, call-heavy abort/segfault rows |
| 3 | layout | Packed aggregate, bitfield, and field-offset runtime evidence lane | Targeted reruns with fault address or failed predicate, plus layout-focused backend tests for packed/bitfield cases | `src/strct-pack-1.c`, `src/bf-pack-1.c`, `src/bitfld-4.c`, aggregate-heavy rows |
| 4 | local/global memory | String, alias, pointer, restrict, and global-buffer evidence lane | Targeted reruns that distinguish bad address materialization from library-call effects, plus memory-lowering backend tests | `src/strlen-1.c`, `src/strlen-7.c`, `src/mayalias-3.c`, `src/restrict-1.c` |
| 5 | branch/compare/control flow | Timeout evidence lane | Reruns or traces proving whether execution is stuck in generated loop code, an external call, or a helper; no timeout-limit change | `src/20000224-1.c`, `src/20000731-2.c`, `src/loop-2b.c`, `src/pr24716.c`, `src/pr85582-1.c` |
| 6 | true runtime support | Residual runtime-support candidates after prerequisite owners shrink | Rerun rows after ABI, layout, memory, call-lowering, and control-flow evidence work; open only narrow helper/runtime ideas with concrete residual evidence | Residual IEEE, division/helper, string/library, or timeout rows only after rerun |

## Direct Implementation Candidate

### Call Lowering: External Call Relocation

Initial row: `src/990106-1.c`.

Why this can become a direct follow-up:

- `02_likely_first_owner_map.md` records a high-confidence call-lowering owner
  for this row.
- The log reports `ld.so` failing in `_dl_fixup` because the relocation type is
  not `ELF_MACHINE_JMP_SLOT`.
- The failure happens during dynamic relocation handling, before source-level
  arithmetic, layout, or runtime helper behavior can explain the result.

Suggested proof surface:

- A focused object-emission or backend test that checks the external call
  relocation/call shape.
- A targeted RV64 torture rerun of `src/990106-1.c`.
- If the row reclassifies to abort, segfault, timeout, or pass after the call
  lowering fix, record the new symptom separately instead of treating the old
  loader assertion as resolved runtime support.

## Evidence Lanes Before Implementation

### ABI

Initial rows and families:

- `src/va-arg-20.c`
- `src/vprintf-1.c`
- call-heavy abort and segfault rows listed in `01_runtime_symptom_map.md`

Needed evidence:

- Distinguish argument placement, return-value movement, varargs layout, and
  external-call setup.
- Capture failed source predicate, register state, or a small reduced backend
  case before opening an implementation idea.

Suggested proof surface:

- Focused backend ABI tests for argument homes, return homes, and varargs.
- Targeted torture reruns for the representative rows.
- A before/after row classification showing fewer ABI-shaped abort or segfault
  owners without weakening expected behavior.

### Layout

Initial rows and families:

- `src/strct-pack-1.c`
- `src/bf-pack-1.c`
- `src/bitfld-4.c`
- aggregate-heavy abort and segfault rows

Needed evidence:

- Prove whether field offsets, packed/aligned aggregate layout, bitfield
  extraction, or stack/global aggregate homes create the bad value or address.
- Capture fault address or failed predicate for representative packed and
  bitfield rows.

Suggested proof surface:

- Focused layout backend tests for packed structs, bitfields, and aggregate
  field access.
- Targeted torture reruns for the representative rows.
- A row-level classification update separating layout-owned rows from ABI or
  memory-owned rows.

### Local/Global Memory

Initial rows and families:

- `src/strlen-1.c`
- `src/strlen-7.c`
- `src/string-opt-17.c`
- `src/mayalias-3.c`
- `src/restrict-1.c`
- pointer, alias, restrict, enum, and global-buffer rows

Needed evidence:

- Distinguish bad local slot addressing, global object materialization, pointer
  provenance, and library-call argument setup.
- Avoid classifying all string rows as true runtime support until call and
  memory facts are separated.

Suggested proof surface:

- Focused backend tests for local memory, global data addressing, pointer
  materialization, and source-home freshness where relevant.
- Targeted torture reruns for representative string, alias, pointer, and
  global-buffer rows.
- A row-level classification update that preserves separate memory, ABI, and
  call owners.

### Branch/Compare/Control Flow

Initial rows:

- `src/20000224-1.c`
- `src/20000731-2.c`
- `src/loop-2b.c`
- `src/pr24716.c`
- `src/pr85582-1.c`

Needed evidence:

- Determine whether each timeout is stuck in generated loop code, an external
  call, a runtime helper, or memory-corrupted control state.
- Preserve the existing timeout policy; this lane is evidence and
  implementation ownership, not a timeout-limit change.

Suggested proof surface:

- Targeted timeout reruns with trace, sampled program counter, or reduced loop
  evidence.
- Focused branch/compare/control-flow backend tests if generated loop behavior
  is implicated.
- A classification update that separates miscompiled control flow from true
  runtime or policy discussion.

## Discussion Or Policy Issues

The following are not implementation work from the current evidence:

- Timeout policy changes. The plan explicitly excludes changing the `20s`
  runtime limit or weakening timeout accounting.
- Runtime comparison weakening, expected-output edits, unsupported-marker
  changes, or allowlist filtering.
- A broad "fix runtime mismatches" initiative that groups aborts, segfaults,
  and timeouts under one owner.
- A true-runtime-support umbrella for all string, IEEE, division, helper, or
  timeout-shaped rows before ABI, layout, memory, call-lowering, and
  control-flow prerequisites are rerun.

## Close-Readiness Classification

Idea `618` is close-ready from the documentation side:

- `01_runtime_symptom_map.md` maps the accepted runtime symptoms and evidence.
- `02_likely_first_owner_map.md` maps likely first owners and unresolved rerun
  lanes.
- This file splits follow-up work by owner and proof surface without creating
  broad generic runtime buckets.
- `index.md` summarizes the result and links the answer files.

Any future implementation should be opened as a separate single-owner idea or
as a clearly labeled discussion/policy item.
