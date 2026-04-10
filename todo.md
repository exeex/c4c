# Active Plan Todo

Status: Active
Source Idea: ideas/open/01_parser_namespace_lookup_helper_extraction.md
Source Plan: plan.md

## Todo

- [x] Step 1: Baseline and document the concrete parser and HIR extraction map
- [x] Step 2: Extract parser namespace lookup helpers
- [ ] Step 3: Extract HIR expression-lowering helpers
- [x] Step 4: Run parser and HIR/frontend validation for touched paths

## Notes

- Keep the slice behavior-preserving.
- Record unrelated discoveries back into `ideas/open/` instead of widening the
  active plan.
- Completed this iteration:
  - parser extraction map confirmed two stable seams: parser namespace lookup
    helpers and HIR call/ref-argument lowering helpers
  - extracted shared parser-local helpers for namespace-stack traversal,
    namespace key synthesis, and canonical-name construction in
    `src/frontend/parser/parser_core.cpp`
  - added `namespace_using_nested_visibility_parse.cpp` coverage for nested
    `using namespace` visibility through parser lookup
- Validation status:
  - targeted parser namespace cases passed
  - full-suite regression guard passed:
    `before passed=3317 failed=0`, `after passed=3318 failed=0`
- Next intended slice:
  - Step 3: extract HIR call/member/ref-argument lowering helpers in
    `src/frontend/hir/hir_expr.cpp`
