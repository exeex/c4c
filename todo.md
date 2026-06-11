Status: Active
Source Idea Path: ideas/open/194_prepared_printer_debug_oracle_replacement_planning.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define Route-Native Replacement Diagnostics

# Current Packet

## Just Finished

Completed Step 2 replacement planning for the retained prepared diagnostic and
oracle surfaces inventoried in Step 1. Each retained surface now has either a
route-native replacement diagnostic plan or an explicit retained-oracle
rationale.

Replacement and retention planning by surface:

- Prepared printer surface:
  `prepare::print(const PreparedBirModule&)` should remain a compatibility
  diagnostic until route-native dumps can expose the same semantic route facts
  without reading prepared storage. Route-native replacement output must cover
  semantic facts by route family: Route 1 producer/constant/value-origin and
  final value-home decisions; Route 2 select-chain and direct-global decisions;
  Route 3 memory address, store source, load source, and addressing-mode
  decisions; Route 4 block-entry/current publication decisions; Route 5
  edge/join source and transfer decisions; Route 6 call-use, argument,
  preserved-value, return, and call-boundary decisions; and Route 7 comparison,
  fused-compare, branch-condition, and condition-materialization decisions.
  Formatting replacement can preserve section grouping for readability, but
  section names are not themselves route facts. Target policy such as stack
  layout, frame plan, dynamic-stack restoration, regalloc, storage plans,
  variadic entry plans, carrier plans, inline asm constraints, intrinsic
  lowering, and runtime-helper choices should be emitted as target-policy or
  target-lowering sections, not mixed into route-semantic sections. Prepared
  compatibility facts such as prepared block/value labels and prepared-only
  lookup ids may remain in the prepared printer while route-native diagnostics
  mature. Temporary adapter: a route-native printer may use a narrow bridge
  that maps route-view identities to existing prepared labels solely to keep
  focus and diff stability while both dumps coexist; the adapter must not be a
  semantic authority.

- Prepared printer tests and CLI prepared dump snippets:
  `backend_prepared_printer` and `backend_cli_dump_prepared_bir_*` remain
  retained oracles for the prepared compatibility surface. Their route-native
  replacement should be a new dump oracle, not rewritten expectations on the
  existing prepared dump. The new oracle must assert semantic route content
  independently from formatting: route sections prove facts, focus behavior
  proves selection semantics, and separate target-policy sections prove frame,
  stack, storage, and call-lowering policy. Existing prepared snippets should
  remain until equivalent route-native positive, negative, ambiguous, and
  fallback expectations exist. Temporary adapter: CLI focus options may be
  shared between prepared and route-native dump commands, but expected text for
  prepared compatibility should stay isolated from route-native expected text.

- Backend CLI dump routing:
  `--dump-prepared-bir` remains the prepared compatibility dump. Route-native
  replacement should be a distinct stage or command path that dumps
  route-native diagnostics from route views or target-local route state, not an
  alias that re-labels `prepare::print(...)`. `--dump-mir` and `--trace-mir`
  should continue to select target-local route-debug output where available.
  Semantic route facts belong in the route-native diagnostic stage; MIR shape,
  instruction spelling, register allocation, ABI, and target policy belong in
  MIR summary/trace or target-policy sections. Temporary adapter: backend dump
  dispatch may keep prepared and route-native stages side by side and share
  focus/filter plumbing, but the route-native stage must report its data source
  clearly enough that prepared facts are not mistaken for replacement facts.

- x86 MIR summary/trace route-debug:
  `summarize_prepared_module_routes(...)` and
  `trace_prepared_module_routes(...)` are retained route-debug compatibility
  surfaces until x86 has equivalent route-native summary/trace output. The
  replacement should report grouped route authority from x86 route views or
  target-local lowering state: Route 6 call-use reuse, call-boundary and
  preserved-value decisions; short-circuit, compare, branch, and condition
  routing; memory-return and address/source routing; grouped spill/reload or
  value-home routing; and multi-function rejection or fallback summaries.
  Formatting such as grouped authority tables and trace ordering can remain
  x86-specific. Target policy such as concrete registers, frame offsets,
  instruction selection, and ABI details should be labeled as x86 target
  policy, not route-semantic authority. Temporary adapter: the x86 route-debug
  wrapper may consume existing prepared compatibility views while it also
  exposes route-native authority groups, but tests should distinguish
  compatibility-derived rows from route-native rows.

- Prepared lookup helper oracle:
  `backend_prepared_lookup_helper` remains an explicit retained oracle because
  it tests prepared aggregate semantics, compatibility lookup status, and
  fallback behavior that are not diagnostics by themselves. Route-native
  replacement should be a helper-oracle family over route-native lookup APIs or
  route views, with the same positive, negative, ambiguous, invalid,
  missing-lookup, and fallback classes for call plans, prior preserved values,
  move bundles, frame-address offsets, memory accesses, edge publications,
  source producers, current/block-entry publication facts, same-block
  producer/source facts, select-chain/direct global, store-source publication,
  fused compare, and Route 6 compatibility consumption. Until that exists, the
  prepared helper oracle is retained as a contraction blocker. Temporary
  adapter: route-native helper tests may compare against prepared helper output
  for transition auditing, but prepared helper results must not be treated as
  the route-native source of truth.

- AArch64 diagnostic-adjacent lookup oracles:
  AArch64 tests that thread `PreparedFunctionLookups` through
  `FunctionLoweringContext` remain production-adjacent compatibility oracles,
  not route-native diagnostics. Their replacement belongs in target-local
  route-view or context-construction tests that verify AArch64 receives
  route-native facts for Route 1 producers/constants, Route 2 select/direct
  global, Route 3 memory/source, Route 4 publication, Route 5 edge/join source,
  Route 6 call-use, Route 7 comparison, return chains, and target-local home,
  storage, and move facts. Semantic route facts should be asserted before
  target policy; instruction dispatch, register choice, and assembly details
  should be separate AArch64 lowering expectations. Temporary adapter:
  `FunctionLoweringContext` may carry both prepared and route-native lookup
  handles during migration, but tests should name which authority each
  assertion uses.

- Target wrapper compatibility:
  x86 wrapper tests and RISC-V prepared edge-publication tests remain
  compatibility-only/oracle-only surfaces. They do not need a user-facing
  route-native diagnostic replacement before Step 3, but they need explicit
  retained-oracle rationale: they protect wrapper behavior while target
  lowerers consume prepared compatibility data. Future replacement should move
  assertions to target wrapper inputs built from route-native views and should
  keep compatibility assertions only for adapters that intentionally still read
  prepared lookups. Temporary adapter: target wrappers may expose dual
  prepared/route-native constructors or consumed-plan views, but route-native
  wrapper tests must not depend on prepared lookup mutation to create semantic
  cases.

- Ownership and expected-output references:
  `tests/backend/OWNERSHIP.md` remains the ownership index for retained
  prepared and route-debug surfaces. No separate expected-output files were
  found for the inspected prepared dump or route-debug CLI snippets; inline
  CMake snippets remain retained compatibility expectations. Route-native
  replacement ownership should add new route-native dump/route-debug names
  alongside existing prepared names instead of renaming the prepared tests.
  Temporary adapter: ownership docs can describe coexistence, but ownership
  labels are formatting/maintenance facts rather than semantic diagnostics.

Unclear ownership resolved for Step 2:

- No generic non-x86 route-debug surface was found in Step 1. Treat x86
  summary/trace as the only current target-local route-debug diagnostic
  replacement candidate. AArch64 selected machine-node printing is target
  machine output, not a prepared route-debug replacement, so it is excluded
  from Step 2 diagnostic replacement scope unless a future idea explicitly
  creates an AArch64 route-debug surface.

## Suggested Next

Execute Step 3 by defining equivalent oracle coverage for each retained
surface, including positive, negative, ambiguous, mismatch, and fallback
coverage required before prepared diagnostics or oracles can contract.

## Watchouts

Do not collapse Step 3 into expectation rewrites. The key split from Step 2 is
semantic route facts versus formatting, target policy, prepared compatibility
facts, and temporary adapters. Prepared printer, prepared CLI dump, x86
route-debug, prepared helper, AArch64 lookup threading, and target-wrapper
compatibility expectations all remain retained until equivalent route-native
coverage is explicitly planned.

## Proof

Docs/lifecycle-only packet; no build required and no `test_after.log` produced.
Verification was this canonical `todo.md` planning update against the Step 1
inventory and Step 2 completion check.
