Status: Active
Source Idea Path: ideas/open/443_closed_world_formal_pointer_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Closed-World Formal Authority Inputs

# Current Packet

## Just Finished

Switched lifecycle from `ideas/open/442_pointer_value_memory_provenance_publication.md`
to `ideas/open/443_closed_world_formal_pointer_authority.md`.

Idea 442 is parked, not closed: its internal-only same-module computed-address
formal pointer provenance is valid completed progress, but Step 4 showed
`930930-1::f` has external linkage and still lacks authority for formal pointer
provenance. The new active idea owns the prerequisite closed-world/private/no-
external-caller metadata before 442 can safely extend beyond
`LirFunction::is_internal`.

## Suggested Next

Execute Step 1: audit current function linkage, visibility, callgraph, and
prepared producer metadata to decide what can prove same-module callsites are
complete for formal pointer provenance.

## Watchouts

- Do not publish formal pointer provenance for external-linkage callees from
  observed same-module direct callsites alone.
- Do not mark `930930-1` done while `%lv.param.reg1` and `%mr_TR - 8` rows
  remain `layout_authority=unknown` and
  `range_verdict=unknown_compatible`.
- Do not weaken `prepare::prepared_pointer_value_memory_has_proven_authority`.
- Keep pointer-delta propagation behind proven base formal pointer authority.
- Do not accept or commit the rejected `test_baseline.new.log` full-suite
  baseline refresh with failing `string_authority_guard`.

## Proof

Lifecycle-only switch. Required local check:

```sh
git diff --check
```
