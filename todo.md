Status: Active
Source Idea Path: ideas/open/199_sema_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Fence Static-Eval Enum Compatibility

# Current Packet

## Just Finished

Step 2 fenced the static-eval enum compatibility route in
`StaticEvalIntEnumLookupInput` and `static_eval_lookup_enum`.

Rendered enum lookup is now documented as legacy/no-metadata compatibility
owned by Sema static-eval enum evaluation for older HIR/static-member callers.
The comments record the limitation and removal condition: complete structured
key or TextId misses fail closed before rendered lookup, and the rendered
bridge can be removed once every static-eval enum caller passes structured enum
metadata. The focused metadata test also proves direct named opt-in through
`StaticEvalIntEnumLookupInput::with_rendered_enum_compatibility` remains
available for no-metadata callers.

## Suggested Next

Hand Step 2 back to the supervisor for review, commit, and lifecycle routing.
The likely next plan packet is Step 3, the `same_rendered_type_name_compatibility`
route, unless supervisor review finds another static-eval enum gap.

## Watchouts

- The rendered enum bridge is intentionally retained because HIR/static-member
  no-metadata callers still opt in through the named compatibility API.
- Do not broaden Step 3 into HIR lowerer cleanup unless the supervisor assigns
  that scope; the current plan is Sema-owned compatibility retirement.

## Proof

Passed delegated proof:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^cpp_hir_sema_consteval_type_utils_structured_metadata$"' > test_after.log 2>&1`

Result: passed, 1/1 focused test green. Proof log: `test_after.log`.
