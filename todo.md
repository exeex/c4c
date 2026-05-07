# Current Packet

Status: Active
Source Idea Path: ideas/open/149_template_instantiation_structured_argument_key.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Migrate HIR Late Instantiation Consumers

## Just Finished

Completed the Step 4 Sema audit slice:

- Audited `src/frontend/sema/type_utils.cpp` template argument comparison.
  `same_template_arg_ref` compares value arguments by numeric value and type
  arguments through `type_binding_values_equivalent`; it does not read
  `TemplateArgRef::debug_text`.
- Classified `type_binding_values_equivalent` as the structured/domain identity
  route: it checks `TypeSpec` base/qualifier shape, template-param identity,
  record-def identity, complete or partial text-name metadata, template origin
  keys, recursive template args, and deferred-member owner/member metadata.
  Its rendered-tag path is explicitly a last compatibility fallback only when
  neither side carries structured text metadata.
- Audited Sema canonical/template-arg formatting declarations and definitions.
  `CanonicalTemplateArg`, `format_template_arg`, `format_canonical_type`, and
  `format_canonical_result` are display/debug APIs; no Sema comparison or key
  route depends on their rendered output.
- Audited Sema deferred/NTTP handling in `consteval.cpp`. It first evaluates
  expression carriers and then uses `template_arg_nttp_text_ids` only to forward
  existing NTTP bindings by parameter text; it is not a template-instantiation
  argument equality key.
- Searched Sema for `debug_text`, canonical rendered formatting, raw
  template-arg `TextId`, and `type_binding_values_equivalent` consumers. No
  Sema-side implementation packet is required for Step 4.

## Suggested Next

Next coherent packet: Step 5 HIR late-instantiation audit/migration of
template argument consumers that still read `TemplateArgRef::debug_text`,
including `src/frontend/hir/impl/templates/*`, `hir_types.cpp`,
`hir_functions.cpp`, and related deferred NTTP/value-argument routes.

## Watchouts

- `type_binding_values_equivalent` still retains
  `same_rendered_type_name_compatibility` for TypeSpecs with no structured name
  metadata. This is compatibility-only, not an observed Step 4 implementation
  blocker; final cleanup should happen in the Step 6 string-mirror pass after
  HIR consumers are isolated.
- `validate.cpp::same_type_name_for_validation` has a similar no-metadata
  rendered-tag compatibility fallback for function/type compatibility, not
  template-instantiation key equality.
- `consteval.cpp::lookup_record_layout` may canonicalize a rendered mirror
  through `link_name_texts->find()` and has a bare rendered fallback only when
  no owner index is available. This is record-layout compatibility, not
  template argument identity.
- HIR remains the high-risk boundary: repo-wide search shows many
  `TemplateArgRef::debug_text` reads in HIR template materialization,
  deduction, type resolution, and value-argument code.

## Proof

Audit-only scratchpad update. No implementation files changed, no build/test
command was required, and `test_after.log` was intentionally not touched.
