# HIR Function-Signature Aggregate-Ref Producer Order

Status: Intentionally concluded — no-change route
Type: bounded upstream HIR semantic producer/order blocker
Successor / return route: `ideas/open/849_hir_function_signature_direct_aggregate_ref_carrier.md`, Step 2

## Intended Goal

Create a production HIR semantic/order path that could make an already
module-issued, definition-backed `HirAggregateRef` available before
`Lowerer::lower_function`, independently of 849's carrier/API delivery.

## Conclusion Record

This bounded route is intentionally concluded without implementation and does
not claim the requested capability. The Step 1 trace found that production
callers in `src/frontend/hir/hir_build.cpp` invoke `Lowerer::lower_function`
with only `Node`/template inputs. `lower_function` has no aggregate semantic
fact input and constructs signature types from `TypeSpec`. Direct
module-issued refs exist only on registered `HirStructDef` objects. Thus an
independent pre-`lower_function` fact cannot reach the lowering seam without
either the carrier/API delivery owned by 849 or forbidden `Node`/`TypeSpec`,
record/tag/owner lookup, maps, or reconstruction.

## Preserved Execution State

- Interrupted step: Step 1 — **Establish the production signature-fact seam
  and order**.
- Result: the step's independent producer-order premise is contradicted; no
  code, tests, plan work, or accepted implementation resulted.
- Accepted prior evidence: 849 Step 1 carrier-seam discovery is `109ea13f4`;
  848 Step 2a is `359a9b94b` with focused `frontend_hir_tests` proof.
- No-change probe proof: fresh `cmake --build --preset default` and
  `ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$'`
  passed before this conclusion.

## Exact Return Point

849 resumes at Step 2 — **Add the bounded direct carrier/API**. It must define
the legal carrier/forwarding boundary and have production callers pass an
already-issued HIR definition/ref only where their construction context has
one, otherwise no fact. It must not recover identity from parser, `TypeSpec`,
record/tag/owner data, `Node*` maps, or reconstruction. 848 remains parked at
its unchanged Step 2b until 849 is accepted.

## Reviewer Reject Signals

- Reject treating this conclusion as implementation or as proof that a
  pre-carrier producer path exists.
- Reject parser/`TypeSpec`/`record_def`, owner, tag, text, parser-pointer,
  `Node*` maps, or reconstructed lookup as a direct semantic fact.
- Reject `qtype_from` attachment, occurrence population, LIR work, or 849
  carrier delivery claimed as work completed by this concluded route.
