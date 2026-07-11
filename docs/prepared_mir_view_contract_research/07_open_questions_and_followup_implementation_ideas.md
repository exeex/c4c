# Open Questions And Follow-Up Implementation Ideas

This document answers Step 4's follow-up question: which decisions remain
unsettled before implementation and which source ideas should follow this
research.

The source idea references ideas 589 and 590 as open paths, but the current
repository has them under `ideas/closed/`. This document uses their closure
notes as archived evidence for freshness and publication authority. It does
not reopen those ideas.

## Recommended Defaults For Unresolved Choices

| Decision | Recommended default | Reason |
| --- | --- | --- |
| First implementation shape | Build a reference-only adapter over `PreparedBirModule`. | It creates the boundary without deleting prepared facts, changing target behavior, or requiring a BIR rewrite. |
| First target | x86 first. | Step 1 shows x86 has the smallest live dependency set and a clear entry at `x86::module::emit()`. |
| First x86 surface | Migrate the internal module-emission overload before public API signatures. | `x86::api::emit_prepared_module()` can remain a compatibility wrapper while the internal target code learns the view. |
| Core traversal | Expose both read-only BIR traversal and prepared instruction cursors. | Current MIR still pattern-matches BIR instructions; cursors prevent mixing BIR instructions with facts from another function/block. |
| Optional feature access | Use named feature views with explicit presence checks. | Absence must fail closed rather than letting targets rediscover prepared truth from raw BIR or diagnostics. |
| Derived lookup ownership | Let the view own derived lookup caches. | RV64, AArch64, and x86 all build lookup helpers today; shared view-owned construction avoids target drift. |
| Diagnostics | Put notes, completed phases, route summaries, focus filters, and verifier detail text behind diagnostic/proof views. | Step 3 classifies these as observational. They may explain failure but must not select emitted operands. |
| Producer equivalence | Compare typed view snapshots before textual dumps or MIR/object snapshots. | Doc 05 rejects freezing the full prepared module or relying only on rendered text. |
| Missing feature behavior | Reject the function/module or fall back before target lowering observes an incomplete view. | Mixing raw old facts with a partial view would make ownership ambiguous. |
| Direct dependency gate | Start with review-time grep allowlists, then convert migrated surfaces to compile-time signature checks. | Legacy raw-module users will remain during migration, but new raw dependencies should become visible immediately. |

## Discussion-Required Architecture Decisions

These decisions should be settled before broad implementation. They are not
good first patches because they define ownership boundaries, not just code
shape.

| Architecture decision | Recommended default | Discussion needed before |
| --- | --- | --- |
| View schema versioning and dump format | Version the view schema and dump only view-exposed core, feature, and verifier facts. | Old/new producer comparison and golden dump tests. |
| Stable id policy for a future new producer | Prefer same durable ids where possible; allow deterministic bijection only inside the comparator and never expose it to MIR. | Any new producer work that does not reuse current prepared id allocation. |
| Feature-view granularity | Start with the feature families named in doc 03: calls, variadic, i128/f128, atomics, intrinsics, inline asm, object data, runtime helpers, and storage/regalloc/frame/dynamic-stack families. | Splitting a feature into smaller views or making a feature target-local. |
| Publication and source dependency view shape | Promote only typed source/publication records needed for lowering; keep proof paths and route names in proof/diagnostic views. | Moving edge-publication, select-chain, or branch source routes behind the view. |
| Freshness authority in the view | Treat selected freshness for an admitted source as a required fact; expose missing, ambiguous, stale, wrong-use, or destination-only outcomes as verifier/admission facts. | Any view API that exposes publication or branch source authority. |
| AArch64 context replacement strategy | Replace raw `FunctionLoweringContext::prepared` only after each feature family has a view-backed consumer. | Any attempt to delete the raw pointer in one broad patch. |
| What remains target-local | Keep final opcode selection, scratch registers, ABI register spelling, relocation encoding, and printer mechanics target-local. | Moving prealloc facts out of shared code or deleting current prepared plans. |

## Narrow Implementation Ideas

These are implementation-sized follow-ups once the research set is accepted.
They should avoid broad target rewrites and should not change expectations,
unsupported markers, runtime policy, or allowlists as proof of progress.

| Narrow idea | Scope | Suggested proof |
| --- | --- | --- |
| Implement old-route `PreparedMirViewAdapter` core | Add `PreparedMirCoreView` and `PreparedMirFunctionView` wrappers over `PreparedBirModule`, including view-owned prepared lookups. | Build, adapter tests, and structural equality against current helper output for a small x86 module. |
| Migrate x86 internal module emission to the core view | Keep `x86::api::emit_prepared_module()` as a wrapper, add `x86::module::emit(const PreparedMirCoreView&)`, and migrate target identity, function iteration, data inputs, and the first `consume_plans()` path. | x86 backend subset and assembly snapshot comparison. |
| Add raw dependency gate for migrated MIR surfaces | Add a review script or CI grep allowlist for `PreparedBirModule` and prealloc includes under MIR target directories. | Script proof plus build. |
| Add canonical `PreparedMirView` dump | Dump only schema, target, functions, cursors, core facts, feature-presence summaries, and verifier statuses. | Deterministic dump tests and explicit omission of notes, phases, route names, and debug text. |
| Add structural comparator skeleton | Compare two old-route views first, then use the same comparator for future old/new producer work. | Old-vs-old equality tests plus one intentional difference test. |
| Migrate RV64 text emitter to the core view | Route `emit_prepared_module_text()` through a view-backed wrapper and move per-function lookups behind the view. | RV64 backend text subset and old/view lookup comparison. |
| Introduce AArch64 view-backed context sibling | Add a parallel context type that carries `PreparedMirFunctionView` and feature handles without deleting the raw legacy context yet. | AArch64 build and one lowered-function equivalence check. |

## Interactions With Ideas 589 And 590

Ideas 589 and 590 are closed, but their closure notes should influence the
view contract.

Idea 589, `Direct Edge-Publication Move Freshness Ownership`, established that
direct edge-publication moves require explicit source freshness for the exact
edge-publication source being accepted. Destination bundle legality, complete
source homes, aliases, and local move shape are not source freshness. It added
direct-edge-publication freshness vocabulary and migrated a current-block join
parallel-copy source route to require selected freshness authority.

PreparedMirView interaction:

- edge-publication move records that targets emit are required facts;
- selected direct-edge-publication source freshness should be visible through
  a typed source/freshness view when that source is admitted;
- missing, ambiguous, stale, wrong-value, wrong-use, or destination-only
  freshness outcomes should be verifier/admission facts;
- the view must not let a target accept a direct edge-publication source only
  from destination bundle legality or a structurally complete source home.

Idea 590, `Branch Stack-Load Freshness Contract`, established that branch
stack-load sources require freshness at the branch terminator point. A stack
home, frame slot, stack object, branch payload, or clobber-safety fact is
necessary context but not sufficient authority. Its closure note says typed
and aggregate branch stack-source producer facts remain the next architecture
gap, and it opened idea 592 for that follow-up.

PreparedMirView interaction:

- branch condition/lhs/rhs source views should expose selected branch-point
  freshness only when it exists and matches the exact value/home/use point;
- stack-home-only rows must remain fail-closed at admission;
- branch freshness statuses belong in verifier/admission views, not in
  target-local instruction selection;
- pointer lhs/rhs, typed, aggregate, select, and edge tails should not be
  silently admitted by the MIR view before their producer/publication contract
  exists.

Together, 589 and 590 argue for a `PreparedMirSourceDependencyView` or
feature-local source dependency records that separate:

- selected source identity and freshness facts used by lowering;
- verifier statuses for missing or invalid source authority;
- proof certificates and route diagnostics that explain how the selected fact
  was derived.

They also argue against making raw publication rows, move bundles, stack
homes, or route names standalone lowering authority.

## What Can Be Slimmed Only After The View Lands

The current `PreparedBirModule` should not be slimmed before target consumers
are behind `PreparedMirView`. Deleting or merging fields first would make the
view migration harder to prove and could accidentally weaken contract-first
behavior.

Do not slim yet:

- `module`: split through view accessors first; only later consider whether
  targets still need raw BIR traversal.
- `names`: keep prepared and BIR name tables available until all cursor and id
  access is view-owned.
- `control_flow`, `value_locations`, `stack_layout`, and `addressing`: these
  are core lowering inputs and should remain explicit.
- `frame_plan`, `dynamic_stack_plan`, `call_plans`, `regalloc`, and
  `storage_plans`: migrate to feature views before deciding whether any
  target-local ownership can replace them.
- `store_source_publications`, `call_argument_value_publications`, and
  edge/source dependency records: keep until source/freshness view ownership
  is explicit and compared structurally.
- `object_data`, special carriers, atomics, intrinsics, inline asm, and
  runtime helper facts: migrate through optional views first.
- local-array proof paths, range proofs, checker inputs, route proofs, and
  selected proof artifacts: separate derived required facts from proof
  certificates before deleting anything.
- `completed_phases`, `notes`, prepared printer route text, and target route
  summaries: move behind diagnostic/proof views before slimming diagnostic
  output.

Can be reconsidered after the view lands:

- target-local recomputation of prepared lookups, once the view owns lookup
  construction;
- compatibility projections such as AArch64 flat `machine_nodes`, once
  terminal printing and tests consume the authoritative MIR stream;
- raw module pointers in target contexts, once every use is covered by core,
  feature, verifier, or diagnostic views;
- duplicate dump-only note/phase paths, once canonical view dumps exist.

## Recommended Next Source Ideas

The evidence supports three follow-up source ideas.

1. `PreparedMirView Core Adapter And X86 Entry Migration`

   Scope: implement a reference-only old-route adapter, add x86 internal
   view-taking module emission, and migrate the first `consume_plans()` path
   without changing public backend behavior.

   Why first: it proves the boundary on the smallest target dependency set and
   creates the surface that later gates can protect.

2. `PreparedMirView Equivalence Dump And Comparator MVP`

   Scope: add a canonical view dump and an old-vs-old structural comparator
   for core facts and selected x86 feature facts. Keep diagnostic text out of
   equality.

   Why second: it gives future RV64/AArch64 and new-producer work a proof
   tool without freezing the full `PreparedBirModule`.

3. `PreparedMir Source Dependency And Freshness View Contract`

   Scope: design and implement the typed source/freshness view that carries
   selected edge-publication and branch-stack-load freshness facts while
   keeping missing/invalid statuses as verifier facts and route details as
   diagnostics.

   Why third: ideas 589 and 590 already settled representative freshness
   ownership rules, but the MIR-facing view needs a stable way to expose those
   facts without promoting raw publication rows, stack homes, or route names
   into lowering authority.

If the supervisor wants only one immediate implementation idea, choose the
core adapter plus x86 entry migration. The comparator and freshness/source view
become more valuable once at least one target consumes the core view.
