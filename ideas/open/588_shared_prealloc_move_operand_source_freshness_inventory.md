# Shared Prealloc Move/Operand Source Freshness Inventory

Status: Open
Type: Architecture consumer migration and inventory
Parent: `ideas/closed/587_prepared_value_freshness_authority_mvp.md`
Related:
- `ideas/closed/585_target_abi_contract_and_value_consumption_research.md`
- `ideas/closed/586_uniform_target_register_identity_policy.md`
- `ideas/closed/587_prepared_value_freshness_authority_mvp.md`
Owning Layer: shared prepared/prealloc move and operand source consumers,
freshness authority publication/query, and closure inventory

## Goal

Migrate the next shared-prealloc move/operand source consumers to the
freshness authority model introduced by idea 587, and produce a concrete
remaining-consumer inventory at closure.

This is not a generic "push freshness everywhere" task. It should choose the
smallest useful shared-prealloc slice after 587, wire representative consumers
that currently still rely on local source ordering or implicit prepared homes,
and make any newly exposed architecture gaps explicit in the closure note.

## Why This Exists

Idea 587 introduced a first-class prepared value freshness authority and wired
representative call-argument, move-bundle source, and producer-publication
operand routes. Its closure note intentionally left several nearby consumers
unwired:

- RV64 object-emission value-home helpers still read prepared homes directly
  outside the 587-supported move-bundle classifier path.
- RV64 stack-source, frame-slot, and byval aggregate call-argument paths still
  rely on structured local validators unless freshness candidates are
  published for register sources.
- AArch64 call consumers still use local selected-source and
  prior-preservation checks.
- x86 Route6 consumed-source integration remains separate.
- Shared-prealloc dependency operands, edge-publication moves,
  select-carrier aliases, branch stack loads, typed stack-source
  publications, recovered narrow store-source publications, and pending
  store-global publication routes were reviewed but not wired.

The next useful step is to attack the shared-prealloc side first. That should
reduce target-local patch pressure and give later RV64, AArch64, and x86
consumer migrations a clearer authority surface.

## In Scope

- Audit shared-prealloc move and operand source consumers that already have
  local fail-closed statuses or diagnostics related to source freshness,
  including at least:
  - dependency operand authorities;
  - edge-publication move consumers;
  - branch stack-load authorities;
  - typed stack-source publications;
  - select-carrier or select-alias operand authority if it is adjacent and
    small enough for the same slice.
- Pick a bounded migration subset from that audit and wire it to the shared
  freshness query/helper added by idea 587.
- Publish freshness authority facts only from existing semantic prepared facts;
  do not invent target-local source ownership to make a path pass.
- Preserve or strengthen fail-closed behavior when freshness authority is
  missing, ambiguous, invalid for the use, or only implied by a destination
  bundle.
- Add focused tests or prepared dump assertions proving:
  - a selected migrated consumer accepts an explicit freshness authority;
  - missing freshness still fails closed;
  - ambiguous or stale source authority does not silently fall back to an old
    prepared home;
  - the selected source authority is visible enough for later backend
    debugging.
- Update diagnostics or printer output where needed so closure can distinguish
  unsupported shape, missing source freshness, ambiguous source freshness, and
  local target incompleteness.

## Out Of Scope

- A full migration of all AArch64, RV64, and x86 backend consumers.
- Replacing every prepared value-home lookup in object emission.
- Changing target triple parsing, `TargetProfile`, ABI semantic
  classification, or uniform target register identity policy.
- Redesigning the freshness authority data model unless the selected consumer
  proves the 587 model is insufficient; if that happens, record the gap and
  split it instead of smuggling a broad redesign into this slice.
- Broad move-bundle scheduler rewrites or parallel-copy legality redesign.
- Expectation rewrites, unsupported-marker changes, allowlist edits, or
  runtime-output changes as proof of progress.

## Acceptance Criteria

- The implementation identifies the audited shared-prealloc consumer set and
  names which subset is migrated in this idea.
- At least one shared-prealloc move or operand source consumer that was not
  wired by idea 587 now consults the shared freshness authority before
  accepting a source.
- The migrated consumer rejects missing, ambiguous, or wrong-use freshness
  authority with a precise fail-closed status or diagnostic.
- Existing 587 call-argument, move-bundle source, and producer-publication
  tests continue to pass.
- Focused tests prove the newly migrated consumer is not accepting a source
  merely because a prepared home or destination bundle is internally
  well-formed.
- Any target backend impact is either through the shared prepared/prealloc
  authority or is documented as intentionally deferred.
- No testcase-shaped matching, expectation weakening, unsupported-marker edit,
  or target-local shortcut is used as the proof of progress.

## Closure Note Requirements

Do not close this idea with a generic "future work remains" note. The closure
note must answer each item below explicitly:

1. Which shared-prealloc consumers were audited?
2. Which consumers were migrated to the 587 freshness query in this idea?
3. Which freshness use kinds and source kinds were accepted by each migrated
   consumer?
4. Which missing, ambiguous, stale, or wrong-use freshness cases now fail
   closed, and what diagnostics or statuses report them?
5. Which consumers were intentionally left unwired, and for each one, is it:
   already protected, deferred because the shape is too broad, blocked on a
   missing producer/publication fact, or blocked on a contract design issue?
6. Did implementation expose any new architecture gap not described by 585,
   586, or 587? If yes, name it and say whether it needs discussion, a
   research doc, or a narrow implementation idea.
7. Did implementation expose any target-specific tail for RV64, AArch64, or
   x86? If yes, name the exact consumer family and whether it is ready for a
   follow-up idea.
8. Which tests or prepared dumps prove the migrated consumers use explicit
   freshness authority rather than stale prepared homes or destination-only
   move authority?
9. What concrete follow-up ideas should be opened next, if any?

## Recommended First Cut

Start from the shared-prealloc consumers that already report
`missing_stack_freshness` or similar source-freshness statuses, because those
paths already have fail-closed vocabulary and test anchors. Prefer one or two
representative consumers over a broad sweep.

The expected result is a sharper map, not just a greener route: after closure,
we should know whether the next slice is another shared-prealloc migration,
RV64 stack-source/frame-slot/byval call-argument freshness, AArch64
call-argument freshness migration, x86 Route6 freshness integration, or a
newly discovered contract design discussion.

## Reviewer Reject Signals

- Reject a slice that only routes around a local diagnostic without consulting
  freshness authority.
- Reject accepting a source because the destination bundle is legal when the
  source itself lacks freshness proof.
- Reject backend-local ordering tweaks presented as shared-prealloc authority
  migration.
- Reject broad rewrites that make it impossible to tell which consumer family
  was actually migrated.
- Reject closure notes that do not list wired, unwired, newly exposed, and
  recommended-follow-up consumer families.
