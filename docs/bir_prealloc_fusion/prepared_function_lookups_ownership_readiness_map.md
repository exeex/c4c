# PreparedFunctionLookups Ownership Readiness Map

Status: durable audit artifact for idea 196.

Source idea:
`ideas/open/196_prepared_function_lookups_ownership_readiness_audit.md`

## Summary

`PreparedFunctionLookups` is still an aggregate compatibility and pass-context
surface. This audit found route-native candidates inside some lookup groups,
but it did not find any group that is ready for deletion, privatization, or
aggregate-wide replacement.

Semantic route facts are limited to selected subfacts such as Route 3
memory/source identity, Route 4 publication identity, Route 5 edge/join-source
identity, Route 6 call-use source identity, and Route 7 comparison provenance.
Those facts can move only one consumer or adapter boundary at a time, with
prepared fallback and route/prepared agreement proof.

Target/prepared policy remains outside route ownership. ABI placement, call
records, addressing legality, stack/frame offsets, value homes, move ordering,
publication emission, register/storage choice, and target wrapper spelling
remain prepared or target-owned even when an adjacent semantic source fact is
route-native.

Compatibility adapters are transitional public surfaces. They are useful only
when they preserve prepared fallback, name the route fact used as authority,
and reject absent, ambiguous, mismatched, invalid, or policy-sensitive route
facts. A broad BIR-owned clone of `PreparedFunctionLookups`, a facade rename,
or a target-only wrapper migration would leave the old coupling intact and is
not acceptable progress.

Prepared printer/debug and oracle surfaces remain retained oracles. Prepared
printer rows, `backend_prepared_lookup_helper`, target-wrapper tests, and
AArch64 lookup-threading tests keep public prepared surfaces alive until
route-native diagnostics and oracle coverage prove equivalent success,
negative, ambiguity/conflict, mismatch, and fallback behavior.

## Ownership Table

| Lookup group | Current readers | Ownership classification | Route-view replacement status | Fallback/oracle surfaces | Adapter need | Blockers | Future readiness |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `call_plans` | AArch64 call lowering and call-boundary helpers; x86 `ConsumedPlans`, scalar call-argument helper, direct extern/same-module wrapper paths, ABI placement helpers; prepared call-plan tests and prepared lookup helper oracles. | Target/prepared policy with one semantic Route 6 compatibility-adapter subfact. Whole call plans remain prepared/target-owned. | Partially replaced only for selected Route 6 call-use source identity, with x86 scalar `i32` source reuse already proven. ABI register/stack placement, variadic FPR counts, byval lanes, clobber/preserve sets, outgoing stack sizing, helper protocols, result lanes, and call-record spelling are not route facts. | `PreparedCallPlanLookups`, indexed call plan/argument/result helpers, call-boundary tests, `backend_prepared_lookup_helper`, x86 route-debug compatibility rows, and AArch64 call oracles. | Future adapters must choose one call instruction plus one argument/result role, prove source id/status agreement, reject absent/mismatched/ambiguous/ABI-bound cases, and fall back to prepared call plans for policy-sensitive facts. | Whole-plan substitution would move ABI/layout policy into route ownership; result-lane and publication-routing consumers remain prepared-coupled. | Ready only for the existing x86 Route 6 scalar source boundary. Future work can target one semantic call-use source read at a time; no deletion, privatization, or aggregate-wide replacement is ready. |
| `address_materializations` | AArch64 globals, memory, calls, address materialization lowering, frame-address offset helpers, prepared-address tests, and prepared lookup helper coverage. | Target/prepared policy. | No route-view replacement is ready for the group. Address materialization combines target-addressing policy, frame/stack object offsets, TLS/global relocation choice, pointer materialization, base-plus-offset legality, volatile/address-space behavior, and memory operand formation. | `PreparedAddressMaterializationLookups`, frame-address offset helpers, address-materialization tests, and prepared lookup helper coverage. | No current adapter boundary. A future packet would need one semantic address/source question that explicitly excludes target address policy and preserves prepared fallback. | Current readers mostly ask target address policy, not separable semantic route identity. | Retain prepared ownership. Future audit can inspect one AArch64 reader to determine whether it asks a semantic identity question or only target policy. |
| `memory_accesses` | AArch64 memory, calls, dispatch/store-source, same-block global-load and load-local helpers; x86 local-slot and compare-load candidates; memory/frame/stack tests; prepared lookup helper oracles. | Mixed: route-native candidate for Route 3 semantic memory/source identity, retained prepared/target ownership for address formation and target policy. | Partial candidate only. Route 3 can own selected memory access id and source value/global/local identity, but AArch64 production readers and policy-sensitive fallbacks still read prepared lookups. | `PreparedMemoryAccessLookups`, `find_prepared_memory_access(...)`, global-load/same-block/load-local helpers, store-source publication helper inputs, memory/frame/stack oracles, and prepared lookup helper coverage. | One selected Route 3 memory/source consumer at a time, proving memory access id, source identity, block/instruction compatibility, ambiguity rejection, prepared mismatch fallback, and target-policy exclusion. | Address formation, frame/TLS/global relocation, volatile/address-space behavior, and materialization policy remain prepared/target-owned. | Future work can target one direct AArch64 `memory_accesses` reader first; x86 wrapper reuse should wait for AArch64 Route 3 oracle-backed proof. Not deletion-ready. |
| `move_bundles` | AArch64 edge-copy, call, return, dispatch, publication, value-materialization, and prologue/move helpers; x86 move/return/instruction wrapper paths; prepared move-bundle tests and helper oracles. | Target/prepared policy and transient pass context. | No route-view replacement is ready. Route 5 or Route 6 may identify adjacent semantic source facts, but move bundles carry scheduling and storage policy. | `PreparedMoveBundleLookups`, move-bundle oracle tests, before-call/before-return/after-call move tests, edge-copy/move-policy oracles, and prepared lookup helper coverage. | Only a future consumer-specific source-identity adapter is plausible; it must fall back to move-bundle lookups for ordering, storage, ABI moves, cycle temporaries, and execution sites. | Move ordering, out-of-SSA placement, cycle temporaries, homes, storage, ABI move phases, and final move records are not BIR route facts. | Retain prepared/target ownership. Future audit should isolate one reader and reject migration if it consumes move policy. |
| `value_homes` | AArch64 operands, prologue, dispatch, publication, memory, calls, select/materialization, and context lookup readers; x86 decoded-home, formal-publication, ABI placement, value-location and register-home helpers; riscv edge-publication operand rendering; BIR/prealloc value-home oracles. | Target/prepared policy. | No route-native contraction boundary is ready. Existing x86 decoded-home use with `value_home_lookups = nullptr` is not deletion evidence. | `PreparedValueHomeLookups`, decoded-home/formal-publication inputs, prepared-memory-operand tests, value-home oracles, target-wrapper tests, and prepared lookup helper coverage. | A future adapter must target one semantic value-id or source-id read, prove route/prepared agreement and fallback, and leave storage/home lookup as prepared-owned. | Storage authority, register choice, stack slot/offset, rematerialization spelling, home rendering, and value-home policy remain prepared/target-owned. | Retain prepared/target ownership. Future work can identify one semantic route-id consumer, but no field contraction is ready from this audit. |
| `edge_publications` | AArch64 dispatch edge copies, memory, calls, publication lowering, block-entry/current-block helpers; x86 edge-publication wrapper paths; riscv edge-publication setup and emission; prepared edge-publication tests and helper oracles. | Mixed: route-native candidate for Route 4 publication identity and Route 5 edge/join-source identity; retained prepared/target ownership for publication construction and emission policy. | Partial candidate only. AArch64 Route 4/5 facts can replace selected semantic reads, but direct AArch64 readers plus x86/riscv prepared wrappers keep the group public. | `PreparedEdgePublicationLookups`, indexed/unique publication helpers, block-entry parallel-copy helpers, stack-source and publication-match helpers, AArch64 dispatch/memory/publication fallbacks, x86 prepared dispatch, riscv prepared emission, wrapper tests, and publication oracles. | One edge-publication consumer or wrapper at a time. x86/riscv adapters must prove predecessor/successor or block-label agreement, destination/source value agreement, duplicate/ambiguous/conflict rejection, and prepared fallback while preserving emitted move/register/stack policy. | Publication record construction, block-order emission, move/homes/storage policy, stack-source extension, and final edge-copy records remain target/prepared-owned. | Strong future candidate for one wrapper boundary after exact AArch64 Route 4/5 semantic proof. Not deletion-ready. |
| `edge_publication_source_producers` | AArch64 dispatch producers, dispatch/publication/memory/calls, store-source and publication-source helpers; prepared printer `select_chains` output; source-producer helper tests; x86/riscv wrapper-adjacent publication/source paths; route-debug and lookup helper oracles. | Mixed: route-native semantic subfact candidates plus retained production fallback and retained printer/debug oracle. | Partial candidates exist for Route 1 producer/constant, Route 2 select-chain/direct-global, Route 5 join-source, Route 6 call-use source, and Route 7 comparison provenance. No whole-group replacement is ready. | Source-producer helpers, prepared printer `select_chains` output, dispatch/lookup/store-source/prepared-memory/call/frame-stack oracles, route-debug compatibility rows, and helper fallback coverage. | One route family and one consumer role at a time, proving route/prepared agreement, absent/mismatched/ambiguous fallback, and policy exclusion. Printer/debug replacement must be separate from production lowering. | Materialization sequence choice, direct-global policy when coupled to call/memory/publication contexts, storage/home/move policy, call/publication routing policy, and final records remain prepared/target-owned. | Best future path is a route-native helper or printer oracle for one semantic lookup family before contracting any public source-producer diagnostic surface. Not deletion-ready. |

## Contraction Prerequisites

Before any aggregate contraction, field privatization, or compatibility adapter
work proceeds, the follow-up plan must name:

- One lookup group and one consumer or adapter boundary.
- The exact semantic route fact being used as authority.
- The retained prepared fallback and oracle surface.
- The target/prepared policy that remains out of route ownership.
- Positive, negative, missing-input, invalid-input, ambiguity/conflict,
  mismatch, and fallback proof cases for that boundary.
- Existing prepared expectations that must remain unchanged.

If the follow-up cannot name each item, the correct classification is retained
prepared surface or blocked/unknown, not contraction readiness.

## Future Work Boundaries

| Boundary | Scope | Minimum proof shape |
| --- | --- | --- |
| Route 6 call-use source adapter | One call argument or result source reader, not a whole call-plan migration. | Route/prepared source agreement, absent/mismatched/ambiguous/ABI-bound fallback, and unchanged call ABI policy. |
| Route 3 memory/source adapter | One AArch64 memory/source identity reader before target-wrapper reuse. | Memory access id and source identity agreement, block/instruction compatibility, prepared mismatch fallback, and unchanged addressing policy. |
| Route 4/5 edge-publication adapter | One AArch64, x86, or riscv edge-publication semantic source boundary after exact route proof. | Predecessor/successor or block-label agreement, destination/source agreement, duplicate/ambiguous/conflict rejection, prepared fallback, and unchanged move/register/stack policy. |
| Route 1/2/5/6/7 source-producer diagnostic | One source-producer semantic lookup family or printer/debug row. | Route-native diagnostic/oracle success and failure coverage beside unchanged prepared printer/helper expectations. |
| Value-home or move-bundle semantic-id audit | One consumer that might ask only value/source identity. | Proof that the consumer does not use home/storage/move policy, plus prepared fallback and unchanged target rendering or move scheduling. |

## Reject Signals

Reject future work as route drift if it:

- Renames `PreparedFunctionLookups`, moves fields, or builds a BIR-owned clone
  without reducing one real residual consumer boundary.
- Deletes or privatizes a lookup group while production, printer/debug,
  target-wrapper, or oracle readers remain.
- Treats target/prepared policy as BIR-owned because a nearby route consumer
  moved.
- Weakens prepared printer, route-debug, wrapper, helper, unsupported-marker,
  c_testsuite, or baseline expectations to make a field look ready.
- Replaces semantic route proof with output relabeling or fixture-shaped
  shortcuts.
- Opens broad `PreparedBirModule` retirement or draft 155 work instead of
  field-level lookup-group retirement analysis.
