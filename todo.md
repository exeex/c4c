# Current Packet

Status: Complete
Source Idea Path: ideas/open/144_member_template_constructor_specialization_metadata.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Reintegrate With Parent TypeSpec-Tag Validation

## Just Finished

Step 8 reintegration record is complete for the constructor-specialization
decomposition plan.

Step 5 completed in commit `0d653f97d` as a combined HIR repair for
member-template constructors.
Instantiated template structs now register all concrete-owner constructor
overloads before lowering constructor bodies, and constructor calls lower the
selected overload through a call-specific specialization path.

The specialization path consumes structured parser/HIR carriers directly:

- Concrete owner template bindings are stored on the constructor overload entry.
- Constructor overload lowering deduces method-template type packs and NTTP
  packs from structured callable parameter and argument `TypeSpec`
  / `TemplateArgRef` metadata.
- Signature substitution expands nested type-pack arguments inside aggregate
  parameter types such as `Owner<Pack...>`, so selected constructor bodies see
  concrete aggregate parameters instead of unresolved pack-owned structs.

This repairs the previous two-phase probe boundary: delegating constructor
lookup can now find the sibling constructor for the concrete owner and lower the
selected member-template constructor body with concrete type-pack/NTTP-pack
bindings.

Plan-owner review after Step 5 advances directly to Step 8. The Step 5 repair
also satisfied the planned Step 6 NTTP-pack and Step 7 selected-constructor-body
lowering boundaries enough that no Step 6/7 blocker is observable in the
positive-Sema proof.

Reintegration facts:

- The delegated `^cpp_positive_sema_` proof is green at 884/884 after the
  constructor-specialization repair.
- The full-suite candidate remained red with 24 known failures and was rejected
  as uncanonical, so it is not acceptance proof for this decomposition close.
- No constructor-specialization-specific failure remains to keep this
  decomposition idea open.

## Suggested Next

Lifecycle action: close this constructor-specialization decomposition idea as
complete, then resume the parent TypeSpec-tag deletion Step 6 / broad validation
route. The remaining full-suite red state belongs to parent-route validation or
a separate follow-up idea, not this completed decomposition packet.

## Watchouts

The two-phase registry is intentionally paired with selected constructor
specialization. Keeping only the registry half would regress back to the
observed `MemberExpr base has no struct tag (field='value')` boundary.

The repair uses structured template parameter indices/text ids and
`TemplateArgRef` value/type carriers. It does not recover constructor semantics
from rendered names, `_t` suffixes, `tag_ctx`, module dumps, or named testcase
patterns.

Do not treat the rejected full-suite baseline candidate as canonical proof. It
is red with 24 known failures and should only guide parent-route reintegration
triage until a supervisor-selected canonical validation command is recorded.

## Proof

Delegated positive-Sema proof:
`cmake --build build && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' > test_after.log 2>&1`

Result: passed, 884/884. This improves the current `test_before.log` baseline
of 883/884 with no new failures in the delegated positive-Sema subset.

Proof log: `test_after.log`.

Full-suite baseline candidate: red with 24 known failures and rejected as
uncanonical. Do not recreate `test_baseline.new.log`; use the parent
TypeSpec-tag deletion route to establish the next canonical broad-validation
baseline.
