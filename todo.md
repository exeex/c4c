Status: Active
Source Idea Path: ideas/open/150_edge_publication_lookup_declaration_owner.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Declaration And Type Dependencies

# Current Packet

## Just Finished

Completed plan Step 1 inspection for idea 150. Mapped the current
`make_prepared_edge_publication_lookups` declarations and definitions, the
signature type dependencies, and the narrow declaration owner to use before any
code edits.

Current public helper declarations in
`src/backend/prealloc/prepared_lookups.hpp`:

```cpp
[[nodiscard]] PreparedEdgePublicationLookups make_prepared_edge_publication_lookups(
    const PreparedNameTables& names,
    const PreparedControlFlowFunction& function,
    const PreparedValueLocationFunction* value_locations,
    const PreparedValueHomeLookups* value_home_lookups = nullptr);

[[nodiscard]] PreparedEdgePublicationLookups make_prepared_edge_publication_lookups(
    const PreparedBirModule& prepared,
    const PreparedControlFlowFunction& function,
    const PreparedValueLocationFunction* value_locations,
    const PreparedValueHomeLookups* value_home_lookups = nullptr);
```

Current definitions in `src/backend/prealloc/prepared_lookups.cpp`:

- internal implementation overload at lines 1491-1497 takes
  `const PreparedBirModule* prepared`, `const PreparedNameTables& names`,
  `const PreparedControlFlowFunction& function`,
  `const PreparedValueLocationFunction* value_locations`, and
  `const PreparedValueHomeLookups* value_home_lookups`;
- public `PreparedNameTables` overload at lines 1654-1659 forwards with
  `prepared == nullptr`;
- public `PreparedBirModule` overload at lines 1663-1668 forwards with
  `&prepared` and `prepared.names`.

Signature dependency mapping:

- `PreparedEdgePublicationLookups`: defined in
  `src/backend/prealloc/publication_plans.hpp` at line 326.
- `PreparedNameTables`: defined in `src/backend/prealloc/names.hpp` at line 31.
- `PreparedControlFlowFunction`: defined in
  `src/backend/prealloc/control_flow.hpp` at line 210.
- `PreparedValueLocationFunction`: defined in
  `src/backend/prealloc/value_locations.hpp` at line 121.
- `PreparedValueHomeLookups`: defined in
  `src/backend/prealloc/value_locations.hpp` at line 127.
- `PreparedBirModule`: defined in `src/backend/prealloc/module.hpp` at line 38,
  and already forward declared in `publication_plans.hpp` at line 856.

Chosen declaration owner:
`src/backend/prealloc/publication_plans.hpp`.

Required direct includes for that owner:

- already direct: `names.hpp` for `PreparedNameTables`;
- already direct: `control_flow.hpp` for `PreparedControlFlowFunction`;
- already direct: `value_locations.hpp` for `PreparedValueLocationFunction`
  and `PreparedValueHomeLookups`;
- no `module.hpp` include is required because the overload only takes
  `const PreparedBirModule&`, and `publication_plans.hpp` already forward
  declares `PreparedBirModule`;
- no new `select_chain_lookups.hpp` dependency is required.

Why this owner is narrow:
`publication_plans.hpp` already owns the edge-publication lookup data model,
including `PreparedEdgePublicationLookups` and adjacent lookup/query helpers.
Adding only the two public `make_prepared_edge_publication_lookups`
declarations there keeps the declaration beside its return type and direct
edge-publication APIs. It does not make `publication_plans.hpp` a replacement
umbrella for `prepared_lookups.hpp` because it should not absorb
`PreparedFunctionLookups`, return-chain lookups, address/materialization
lookups, or architecture facade declarations. A new narrowly named prealloc
header is not needed for Step 2 because it would either need to include
`publication_plans.hpp` for real consumers or become a thin redeclaration
wrapper around the actual edge-publication owner.

Inspection commands/results:

- `rg -n "make_prepared_edge_publication_lookups|PreparedEdgePublicationLookups|PreparedNameTables|PreparedControlFlowFunction|PreparedValueLocationFunction|PreparedValueHomeLookups|PreparedBirModule" src/backend/prealloc/prepared_lookups.hpp`
  found the two public declarations and the aggregate facade fields.
- `rg -n "make_prepared_edge_publication_lookups|PreparedEdgePublicationLookups|PreparedNameTables|PreparedControlFlowFunction|PreparedValueLocationFunction|PreparedValueHomeLookups|PreparedBirModule" src/backend/prealloc/prepared_lookups.cpp`
  found the internal pointer overload plus the two public forwarding
  definitions.
- `sed -n '1,240p' src/backend/prealloc/publication_plans.hpp` and
  `sed -n '240,620p' src/backend/prealloc/publication_plans.hpp` confirmed the
  header owns the edge-publication lookup types and already includes the
  signature dependency headers.
- `sed -n '820,940p' src/backend/prealloc/publication_plans.hpp` confirmed the
  existing `PreparedBirModule` forward declaration.
- `sed -n '1,240p' src/backend/prealloc/select_chain_lookups.hpp` confirmed
  `select_chain_lookups.hpp` is a consumer of `publication_plans.hpp` and is
  not the right owner for constructing edge-publication lookup tables.
- `rg -n "#include .*prepared_lookups\\.hpp" src/backend` found the motivating
  direct AArch64 include plus remaining facade users for later steps.

## Suggested Next

Execute plan Step 2 by moving the two public
`make_prepared_edge_publication_lookups` declarations from
`prepared_lookups.hpp` into `publication_plans.hpp`; keep
`prepared_lookups.hpp` as the aggregate prepared lookup facade and include the
narrow owner there only if needed for existing facade users.

## Watchouts

Do not move the internal pointer overload from `prepared_lookups.cpp`; it is an
implementation detail. Do not add `module.hpp` to `publication_plans.hpp` for
this helper. After Step 2, `dispatch_producers.cpp` may still need both
`publication_plans.hpp` for `PreparedEdgePublicationLookups` and
`select_chain_lookups.hpp` for select-chain helpers; do not rely on one to
hide the other transitively.

## Proof

Inspection-only packet per supervisor proof. No build or tests were required
because only `todo.md` was edited; no `test_after.log` was produced.
