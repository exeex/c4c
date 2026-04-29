Status: Active
Source Idea Path: ideas/open/128_parser_ast_handoff_role_labeling.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Current AST Carrier Layout

# Current Packet

## Just Finished

Step 1 inspected `src/frontend/parser/ast.hpp` and mapped the current AST carrier
layout before comment edits.

`TypeSpec` current field-role map:
- Source/semantic type identity: `base`, `enum_underlying_base`, `tag`,
  `record_def`, `qualifier_segments`, `n_qualifier_segments`,
  `is_global_qualified`.
- Declarator shape: `ptr_level`, `is_lvalue_ref`, `is_rvalue_ref`,
  `array_size`, `array_rank`, `array_dims`, `is_ptr_to_array`, `inner_rank`,
  `array_size_expr`, `is_fn_ptr`.
- Qualifiers/attributes/layout hints: `align_bytes`, `is_const`,
  `is_volatile`, `is_packed`, `is_noinline`, `is_always_inline`,
  `is_vector`, `vector_lanes`, `vector_bytes`.
- Template/member-dependency handoff: `tpl_struct_origin`,
  `tpl_struct_args`, `deferred_member_type_name`.
- Compatibility/legacy notes: `tag`, `qualifier_segments`, and
  `deferred_member_type_name` are spelling strings that cross stages; comments
  should make clear when they are identity/display/legacy bridge payload versus
  structured authority. `record_def` is already the concrete parser record
  identity for struct/union types.

`TemplateArgRef` current field-role map:
- Structured argument contract: `kind`, `type`, `value`.
- Debug/display/recovery spelling: `debug_text`. This string is consumed by
  helper paths such as template-param mention/debug rendering and should be
  labeled as compatibility/debug text, not primary semantic authority.

`Node` current field-role map:
- Node identity/source location: `kind`, `line`, `column`, `file`,
  `namespace_context_id`.
- Typed semantic handoff: `type`, `builtin_id`, declaration/function pointer
  payload (`params`, `n_params`, `fn_ptr_params`, `n_fn_ptr_params`,
  `fn_ptr_variadic`, `ret_fn_ptr_params`, `n_ret_fn_ptr_params`,
  `ret_fn_ptr_variadic`), record/enum payload (`fields`, `n_fields`,
  `member_typedef_names`, `member_typedef_types`, `n_member_typedefs`,
  `base_types`, `n_bases`, `pack_align`, `struct_align`, `enum_names`,
  `enum_name_text_ids`, `enum_vals`, `n_enum_variants`,
  `enum_has_explicit`), and constructor/lambda metadata.
- Name authority and spelling bridge: `name`, `unqualified_name`,
  `unqualified_text_id`, `qualifier_segments`, `qualifier_text_ids`,
  `n_qualifier_segments`, `is_global_qualified`.
- Expression/statement carrier shape: `op`, `ival`, `fval`, `sval`,
  `is_imaginary`, `left`, `right`, `cond`, `then_`, `else_`, `body`, `init`,
  `update`, `children`, `n_children`.
- Template payload: `template_param_names`, `template_param_name_text_ids`,
  `template_param_is_nttp`, `template_param_is_pack`,
  `template_param_has_default`, `template_param_default_types`,
  `template_param_default_values`, `template_param_default_exprs`,
  `n_template_params`, `template_arg_types`, `template_arg_is_value`,
  `template_arg_values`, `template_arg_nttp_names`, `template_arg_exprs`,
  `n_template_args`, `has_template_args`, `template_origin_name`.
- Flags and semantic hints: `variadic`, `is_static`, `is_extern`, `is_inline`,
  `is_constexpr`, `is_consteval`, method/ref/operator flags, concept/specialization
  flags, visibility/asm/member/designator/constructor/default/delete/pack flags,
  `linkage_spec`, `operator_kind`, `execution_domain`,
  `lambda_capture_default`, `lambda_has_parameter_list`.
- Compatibility/rendered-spelling fields needing explicit labels:
  `name`, `unqualified_name`, `qualifier_segments`, `template_param_names`,
  `template_param_default_exprs`, `template_arg_nttp_names`,
  `template_origin_name`, `member_typedef_names`, `enum_names`,
  `asm_constraints`, `desig_field`, `linkage_spec`, `op`, and `sval`.
  `TextId` parallels mark parser-owned identity for some spellings, while the
  raw strings remain display/legacy bridge inputs in several cross-stage paths.

## Suggested Next

Delegate Step 2 to an executor: add behavior-preserving role labels for
`TypeSpec` and `TemplateArgRef` in `src/frontend/parser/ast.hpp`, explicitly
separating structured cross-stage payload from spelling/debug/compatibility
fields.

## Watchouts

- Keep this runbook behavior-preserving.
- Do not replace string lookup paths or change parser/Sema/HIR/codegen behavior.
- Record suspicious cross-stage string authority as follow-up idea work instead
  of fixing it ad hoc.

## Proof

Inspection-only todo update; delegated proof said no build proof required and no
tests were run. `test_after.log` was left unchanged.
