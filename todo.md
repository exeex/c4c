# Current Packet

Status: Complete
Source Idea Path: ideas/open/96_sema_dual_lookup_structured_identity_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 8
Current Step Title: Demote Sema-Owned Legacy String Paths

## Just Finished

Step 8 - Demote Sema-Owned Legacy String Paths: inventoried retained sema
string fallbacks and demoted the scoped-local symbol path that is sema-owned,
structured-stable, and not a HIR/diagnostics/consteval/codegen bridge.

Local `TextId` scope mirrors now reject qualified/global references, then
`lookup_symbol` prefers the structured scoped binding when present. Local
initialization marking can also update the structured binding when a rendered
lookup misses. The rendered local scope map remains populated for same-scope
redefinition diagnostics, predefined locals, `this`, template NTTP names, and
nodes that lack usable `TextId` metadata.

Retained fallback classification:

- Sema-owned demoted: unqualified local symbol lookup and local initialization
  marking now use the structured scoped mirror first when a valid local
  `TextId` key exists.
- Diagnostic/redefinition bridge: rendered local scope names remain required
  for same-scope redefinition checks and user-facing names.
- Bridge-required: rendered globals, functions, overload sets, and consteval
  function registries remain populated for diagnostics, HIR handoff, consteval
  call paths, and call sites without stable structured metadata.
- Consteval bridge-required: string constant maps, interpreter locals, NTTP
  bindings, and type bindings remain behavior sources because HIR compile-time
  evaluation and `TypeSpec::tag` still consume rendered names.
- HIR-blocked: `TypeSpec::tag`, complete struct/union sets, record member maps,
  base-tag lists, and tag-to-record bridges remain rendered-authoritative until
  record/type identity carries stable structured metadata across HIR-facing
  boundaries.
- Legacy-proof/advisory: structured enum, const-int, consteval, type-binding,
  function/global, overload, and record mirrors remain dual-read proof paths
  where the plan intentionally preserved rendered behavior.
- Not lookup fallbacks: canonical symbol formatting, mangling/link names,
  diagnostics, and string literal helpers are rendered surfaces, not removable
  sema lookup fallbacks in this packet.

## Suggested Next

Handoff to the supervisor/plan-owner to decide whether Step 8 closes the active
runbook or should spawn a separate HIR-facing identity cleanup idea. No further
sema-owned string fallback was identified as both removable and independent of
HIR, diagnostics, consteval, or codegen bridges.

## Watchouts

- Do not remove `Node::name`, `TypeSpec::tag`, rendered diagnostics, mangled
  names, link names, or HIR compile-time-engine string maps in this plan.
- Global/function/overload structured mirrors remain advisory because rendered
  maps still define bridge behavior and diagnostics.
- Enum variant nodes still lack per-variant `TextId` metadata, so enum
  constants remain rendered-authoritative for behavior.
- Template parameter and NTTP source metadata is still incomplete at several
  sema/HIR boundaries, so type binding and NTTP string maps are bridge-required.
- Record completeness/member/static-member mirrors still depend on rendered
  `TypeSpec::tag` bridges and ambiguity filtering.
- The local structured demotion is limited to unqualified identifiers; qualified
  or global-qualified references intentionally do not produce local keys.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^positive_sema_' > test_after.log 2>&1`

Result: passed. `test_after.log` records 34/34 `positive_sema_` tests passing.
