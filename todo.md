Status: Active
Source Idea Path: ideas/open/445_closed_world_no_external_caller_metadata_source.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Classify Possible Metadata Sources

# Current Packet

## Just Finished

Completed Step 1: classified possible metadata sources for
closed-world/no-external-caller authority.

Source table:

| Source candidate | Classification | Reason |
| --- | --- | --- |
| Static/internal linkage | Accepted existing baseline | Already feeds `FormalPointerAuthorityKind::InternalOnly` from `LirFunction::is_internal`; not a `NoExternalCaller` source for non-internal definitions. |
| Explicit CLI/frontend closed-world mode | Missing | `c4cll` currently has target, dump/codegen, optimization, PIC/PIE, include, and split-output flags, but no closed-world/no-external-caller mode. |
| HIR linkage and visibility | Rejected current source; split-needed only with a tested linker contract | HIR carries static/extern/inline/weak/default/hidden/protected, but only static proves internal-only authority today. |
| LIR function/module fields | Rejected current source | `is_internal`, `can_elide_if_unreferenced`, `is_declaration`, `link_name_id`, `extern_decls`, and globals identify module structure but do not prove caller-set completeness. |
| `LinkNameId` / symbol identity | Rejected | Required identity input for any future source, but not no-external-caller authority. |
| `can_elide_if_unreferenced` and dead-reachability pruning | Rejected | Local discardability/dead-code pruning is not a formal no-external-caller proof for non-internal definitions. |
| Observed same-module direct calls | Rejected | Call observations are not complete for externally callable definitions. |
| Function-address escape tracking | Missing/incomplete | Existing metadata preserves some function references, but there is no complete escaped-function summary. |
| Indirect-call target information | Missing/incomplete | Calls can be marked indirect, but no target set or target-exclusion proof exists. |
| Prepared/BIR surfaces | Rejected as source; valid consumer boundary | BIR/prepared carries and consumes authority; it must not invent the source from dump rows, callsite counts, or shape. |
| Linker/runtime visibility semantics | Missing/split-needed | Visibility text exists, but no tested compiler/linker contract equates it to no external caller. |
| Whole-program/LTO assumption | Missing/split-needed | Plausible future source, but no mode or metadata exists today and it must cover address escape plus indirect calls. |

First executable packet:

- No current implementation packet can soundly populate
  `FormalPointerAuthorityKind::NoExternalCaller`.
- Step 2 should define the selected-source contract as a rejection in the
  current compiler model and state the split requirements for a future source,
  likely a whole-program/LTO-style mode or a tested linker/visibility contract.
- `930930-1::f` remains fail-closed.

Artifact:

- `build/agent_state/445_step1_metadata_source_classification/classification.md`

## Suggested Next

Execute Step 2: Define Selected Source Contract Or Rejection.

Recommended Step 2 packet: record that no current metadata source is accepted
for non-internal `NoExternalCaller`; keep the authority false by default; and
recommend plan-owner split if a new whole-program/visibility metadata source is
desired.

## Watchouts

- Do not infer authority from observed same-module direct calls, visibility
  spelling, `LinkNameId`, `can_elide_if_unreferenced`, absent extern
  declarations, or testcase shape.
- Account for function-address escape and indirect-call target exclusion before
  accepting any metadata source.
- Keep `930930-1::f` fail-closed unless a real source proves no outside caller
  can provide different pointer values.
- Do not weaken `prepare::prepared_pointer_value_memory_has_proven_authority`.
- Do not accept or modify `test_baseline.new.log`.

## Proof

Step 1 classification-only check:

```sh
git diff --check
```
