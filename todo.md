Status: Active
Source Idea Path: ideas/open/194_prepared_printer_debug_oracle_replacement_planning.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Define Equivalent Oracle Coverage

# Current Packet

## Just Finished

Completed Step 3 equivalence coverage planning for the retained prepared
diagnostic and oracle surfaces. Each retained surface now has named coverage
that must exist before the prepared compatibility surface can contract, plus
explicit preservation rules for existing oracle strength.

Equivalent oracle coverage by route family:

- Route 1 producer, constant, value-origin, and value-home coverage:
  replacement oracles must include positive cases where producer and constant
  facts are present, negative cases where a value has no route-native producer
  fact, ambiguous cases where multiple candidate homes or producers would be
  possible without a tie-breaker, mismatch cases where prepared compatibility
  and route-native facts disagree, and fallback cases where the route-native
  query declines and the compatibility surface remains authoritative.

- Route 2 select-chain and direct-global coverage:
  replacement oracles must cover positive select-chain/direct-global facts,
  negative non-select and non-global paths, ambiguous overlapping select/global
  candidates, mismatch between prepared labels and route-native source facts,
  and fallback when route-native lookup cannot resolve the chain without
  prepared compatibility data.

- Route 3 memory, address, store-source, load-source, and addressing coverage:
  replacement oracles must prove present memory/source facts, absent facts for
  non-memory values, ambiguous alias or addressing candidates, mismatch between
  prepared memory rows and route-native address/source authority, and fallback
  when target lowering still consumes prepared memory lookups.

- Route 4 block-entry/current publication coverage:
  replacement oracles must prove positive block-entry and current-value
  publication, negative no-publication cases, ambiguous publication ownership
  at block boundaries, mismatch between prepared publication rows and
  route-native publication state, and fallback when compatibility publication
  lookup is still the consumed authority.

- Route 5 edge, join-source, and transfer coverage:
  replacement oracles must cover positive edge and join-source facts, negative
  straight-line or non-edge values, ambiguous multi-predecessor ownership,
  mismatch between prepared edge-publication rows and route-native edge facts,
  and fallback where wrappers or target lowerers still depend on prepared edge
  publication.

- Route 6 call-use, argument, preserved-value, return, and call-boundary
  coverage:
  replacement oracles must include positive call-plan and preserved-value
  facts, negative non-call/non-preserved paths, ambiguous reused-call or
  return-chain candidates, mismatch between prepared call plans and
  route-native call-boundary authority, and fallback where compatibility
  helpers still provide the consumed call plan.

- Route 7 comparison, fused-compare, branch-condition, and condition
  materialization coverage:
  replacement oracles must prove positive comparison and branch-condition
  facts, negative non-branch or materialized-condition cases, ambiguous
  compare/branch ownership, mismatch between prepared fused-compare rows and
  route-native branch authority, and fallback where target route-debug still
  consumes prepared comparison lookups.

Equivalent oracle coverage by retained surface:

- Prepared printer surface:
  `prepare::print(const PreparedBirModule&)` can contract only after a
  route-native printer oracle covers the positive, negative, ambiguous,
  mismatch, and fallback classes for Routes 1 through 7 above. Coverage must
  assert semantic facts by route family separately from formatting sections,
  prepared block/value label compatibility, and target-policy details. The
  prepared printer must keep all current compatibility facts until the
  route-native printer proves that each retained fact is either replaced by a
  route-native semantic assertion or intentionally preserved as compatibility
  output.

- Prepared printer tests and CLI prepared dump snippets:
  `backend_prepared_printer` and `backend_cli_dump_prepared_bir_*` can
  contract only after distinct route-native dump tests cover positive route
  facts, negative/focused-out facts, ambiguous focus or route selection,
  mismatch between prepared and route-native expected text, and fallback to the
  prepared dump when route-native data is unavailable. Existing inline CMake
  snippet strength must be preserved: do not weaken prepared expected text, do
  not rename prepared tests into route-native tests, and do not treat formatting
  differences as semantic replacement coverage.

- Backend CLI dump routing:
  `--dump-prepared-bir` can contract only after CLI routing coverage proves a
  separate route-native dump stage. Required coverage is a positive selection
  of the route-native diagnostic stage, negative rejection of accidental
  prepared-stage aliasing, ambiguous behavior when both prepared and
  route-native dump flags or focus filters are present, mismatch reporting when
  the route-native stage would expose facts different from prepared output, and
  fallback behavior that keeps `--dump-prepared-bir` as the compatibility dump.
  Existing CLI oracle strength is preserved by keeping prepared and
  route-native expected text isolated.

- x86 MIR summary/trace route-debug:
  `summarize_prepared_module_routes(...)` and
  `trace_prepared_module_routes(...)` can contract only after x86
  route-native summary/trace coverage proves grouped route authority from x86
  route views or target-local lowering state. Required cases are positive
  Route 6, branch/condition, memory-return, address/source, and value-home
  rows; negative absence of route rows for unsupported or non-applicable MIR
  paths; ambiguous multi-function, multi-edge, or reused-call summaries;
  mismatch between compatibility-derived rows and route-native authority rows;
  and fallback summaries when the x86 wrapper still reads prepared views.
  Existing route-debug strength is preserved by continuing to label
  compatibility-derived rows distinctly from route-native rows until the latter
  owns every asserted semantic fact.

- Prepared lookup helper oracle:
  `backend_prepared_lookup_helper` can contract only after a route-native
  helper-oracle family covers the same semantic classes as the prepared helper:
  call plans, prior preserved values, move bundles, frame-address offsets,
  memory accesses, edge publications, source producers,
  current/block-entry publication facts, same-block producer/source facts,
  select-chain/direct global, store-source publication, fused compare, and
  Route 6 compatibility consumption. For each class, required coverage is
  positive lookup success, negative missing fact, ambiguous competing fact,
  invalid or missing lookup input, mismatch against prepared compatibility
  output, and fallback to prepared helper status. Existing oracle strength is
  preserved by retaining prepared aggregate semantics and fallback status
  assertions until the route-native helper proves the same success and failure
  contracts without using prepared results as the source of truth.

- AArch64 lookup threading:
  AArch64 `PreparedFunctionLookups` threading through
  `FunctionLoweringContext` can contract only after target-local route-view or
  context-construction tests prove that AArch64 receives route-native facts for
  Routes 1 through 7, return chains, target-local homes, storage, and move
  facts. Required coverage is positive receipt and consumption of each fact
  family, negative absence when no route-native fact exists, ambiguous dual
  prepared/route-native authority during migration, mismatch between prepared
  lookup handles and route-native handles, and fallback when AArch64 lowering
  intentionally still consumes prepared compatibility handles. Existing oracle
  strength is preserved by naming the authority used by every assertion and by
  keeping instruction dispatch, register choice, and assembly expectations
  separate from route-fact threading coverage.

- Target-wrapper compatibility surfaces:
  x86 wrapper tests and RISC-V prepared edge-publication tests can contract
  only after wrapper-input coverage built from route-native views proves the
  same wrapper behavior now protected by prepared compatibility data. Required
  coverage is positive construction from route-native route facts, negative
  rejection or absence of unsupported wrapper facts, ambiguous dual-constructor
  or dual-plan cases during migration, mismatch between prepared wrapper input
  and route-native wrapper input, and fallback paths for adapters that
  intentionally still consume prepared lookups. Existing oracle strength is
  preserved by keeping prepared wrapper assertions for adapters until
  route-native wrapper tests no longer depend on prepared lookup mutation to
  create semantic cases.

Coverage gaps that block contraction:

- No retained surface can contract merely because a route-native diagnostic
  prints similar section names. It needs semantic assertions for the relevant
  positive, negative, ambiguous, mismatch, and fallback cases.
- Prepared printer and prepared CLI dump coverage cannot contract until
  route-native dump tests cover all route families currently protected by
  prepared output and keep target-policy details separate.
- x86 route-debug cannot contract until route-native x86 summary/trace tests
  distinguish compatibility-derived rows from route-native authority rows and
  cover fallback summaries.
- `backend_prepared_lookup_helper` cannot contract until route-native helper
  tests reproduce both success and failure/fallback status semantics for every
  helper family listed above.
- AArch64 lookup threading cannot contract until target-local context tests
  prove route-native fact delivery independently from prepared lookup handles.
- Target-wrapper compatibility cannot contract until route-native wrapper
  inputs replace prepared lookup mutation as the way semantic cases are built.
- Ownership docs and expected-output names are not replacement evidence by
  themselves; they only preserve maintenance boundaries while both oracle
  families coexist.

## Suggested Next

Execute Step 4 by deciding which retained prepared diagnostic and oracle
surfaces can be decomposed into independent implementation packets, using the
Step 3 coverage gates as contraction blockers.

## Watchouts

Do not accept expectation rewrites, section-name matching, or ownership-label
updates as equivalent oracle coverage. Existing prepared oracle strength must
remain intact until each affected route family and surface has positive,
negative, ambiguous, mismatch, and fallback coverage from route-native
authority or an explicitly retained compatibility adapter.

## Proof

Docs/lifecycle-only packet; no build required and no `test_after.log` produced.
Verification was this canonical `todo.md` planning update against the Step 1
inventory and Step 2 completion check.
