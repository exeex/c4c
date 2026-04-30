# Parser Known Function Name Compatibility Spelling Cleanup

Status: Closed
Created: 2026-04-29
Closed: 2026-04-30

Parent Ideas:
- [131_cross_ir_string_authority_audit_and_followup_queue.md](/workspaces/c4c/ideas/open/131_cross_ir_string_authority_audit_and_followup_queue.md)

## Goal

Retire parser known-function-name compatibility spelling paths where rendered
function names can still decide declaration and call disambiguation after
`QualifiedNameKey` identity is available.

## Why This Idea Exists

Idea 131 Steps 2 and 3 found that the parser backing table for known function
names is already structured as
`std::unordered_set<QualifiedNameKey> known_fn_names`, but some producer and
consumer paths still route through rendered strings that are reparsed back into
keys. That compatibility spelling can still affect function/call
disambiguation, so it should be narrowed to explicit fallback or display use.

Suspicious paths:

- `has_known_fn_name(const std::string&)`
- `register_known_fn_name(const std::string&)`
- `register_known_fn_name_in_context()` fallback branches that parse rendered
  spelling into a `QualifiedNameKey`
- rendered registration call sites for `qualified_op_name`,
  `qualified_ctor_name`, and `scoped_decl_name`
- call-disambiguation probes that check rendered `head_name`,
  `current_member_name`, or visible-name spelling after a structured key path
  exists

## Scope

- Trace parser known-function-name registration and lookup through declaration
  parsing, namespace/context qualification, visible-name resolution, and
  expression/type disambiguation.
- Prefer `QualifiedNameKey`, `TextId`, parser name paths, and namespace context
  ids as the semantic authority for known function names.
- Keep rendered function names only as diagnostics, AST/final spelling, or
  explicit compatibility fallback with structured-primary behavior and
  mismatch visibility.
- Remove or quarantine string overloads once call sites have structured
  registration or lookup equivalents.

## Out Of Scope

- Changing function declaration syntax, overload semantics, or namespace
  visibility rules.
- Removing final AST function spelling or generated operator/constructor names
  before downstream consumers have structured replacements.
- Folding this work into Sema/HIR owner or member lookup cleanup; those paths
  are covered by ideas 135 and 136.
- Broad parser string cleanup outside known-function-name registration and
  disambiguation.

## Acceptance Criteria

- Known-function-name registration and lookup use structured keys as the
  normal semantic path for free functions, namespace-qualified functions,
  operators, constructors, and visible aliases.
- Any remaining rendered-name overloads are compatibility-only, structured
  primary, and cannot silently decide disambiguation when a structured key is
  available.
- Focused parser/frontend tests cover namespace-qualified functions,
  out-of-class operators, out-of-class constructors, and visible-name
  disambiguation where rendered spelling previously bridged identity.

## Closure Notes

- Completed the active runbook through Step 5.
- Broader parser confidence proof rebuilt `c4cll` and `frontend_parser_tests`
  and passed the selected 44/44 parser/frontend tests.
- Close-time regression guard passed with non-decreasing maintenance semantics:
  `test_before.log` and `test_after.log` both report 44 passed, 0 failed.
- Remaining rendered function spelling is retained only as final AST/display
  data or explicit compatibility fallback, not as the normal known-name
  semantic authority when structured identity is available.
