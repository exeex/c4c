# Current Packet

Status: Active
Source Idea Path: ideas/open/144_member_template_constructor_specialization_metadata.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement Constructor Prelude Carrier

## Just Finished

Step 2 inventoried the member-template constructor specialization seams without
implementation edits.

Observed parser/Sema source surfaces:
- `parse_record_template_member_prelude`,
  `ParserRecordTemplatePreludeGuard`, and
  `try_parse_record_member_with_template_prelude` are the record-member
  prelude boundary.
- `try_parse_record_constructor_member` creates constructor `Node`s, but does
  not receive or attach a complete structured member-template prelude carrier.
- Top-level template declarations already have the richer attachment pattern in
  `declarations.cpp` (`template_param_names`, text ids, NTTP flags, pack flags,
  defaults, and annotation), which is the closest local model.
- `TemplateArgRef::nttp_text_id` and `Node::template_arg_nttp_text_ids` are the
  structured value-template argument carriers for `index_sequence<I1...>` and
  `get<I1>(...)`; value args must keep those ids rather than falling back to
  debug/rendered text.

Observed HIR source surfaces:
- `lower_struct_def` registers concrete struct constructors and pending
  methods with struct-owner bindings only.
- `register_instantiated_template_struct_methods` lowers instantiated template
  struct methods immediately with owner bindings, and does not register a
  constructor-specialization overload family for method-template constructors.
- `try_lower_direct_struct_constructor_call` selects public constructors, but
  does not deduce member-template type packs from constructor call arguments.
- `lower_struct_method` selects delegating constructor overloads, but does not
  specialize the selected constructor with method-template type-pack or NTTP-pack
  bindings before emitting the call.
- `try_deduce_template_type_args`, `build_call_nttp_bindings`, and
  `get_template_param_order` provide reusable binding conventions; current type
  deduction does not recursively match nested template-struct params such as
  `tuple<Args1...>` against concrete `tuple<int>` arguments, and current NTTP
  text binding does not build pack entries such as `I1#0`.

First bad carrier boundary:
the record-member template prelude is parsed as a transient scope, but the
constructor/method `Node` does not reliably get a full structured prelude
carrier for type params, NTTP params, packs, defaults, and value-arg text-id
annotation. This leaves HIR constructor specialization without authoritative
method-template metadata for the public `pair` constructor and the private
delegating constructor.

## Suggested Next

Implement the first coherent repair seam: extend the record-member template
prelude carrier and attach it to created method/constructor `Node`s immediately
after record-member dispatch, following the existing top-level template metadata
shape. Include structured NTTP/pack/default metadata and annotate value-template
argument refs so `index_sequence<I1...>` and `get<I1>(...)` retain
`nttp_text_id` carriers.

Step 3 focused probes have not been added yet: the existing failing
positive-Sema fixture plus the Step 2 read-only inventory are sufficient to
start the first parser carrier seam without another probe-only packet.

After that parser-to-HIR carrier exists, the next implementation packet should
specialize constructor overload selection in HIR: deduce method type packs from
direct constructor-call argument types and bind NTTP packs for delegating
constructor calls using the existing structured binding conventions.

## Watchouts

- Step 2 was inventory-only; no implementation files or test logs were touched.
- Parse-only output can show constructor template names, but the durable carrier
  is still insufficient for full method-template ownership, NTTP packs, and
  constructor specialization handoff.
- Do not reintroduce `TypeSpec::tag` or rendered-string semantic lookup.
- Do not replace missing method-template or NTTP-pack bindings with
  named-case, rendered-name, `_t`, or `tag_ctx` recovery.
- Avoid starting in HIR by recovering metadata from mangled/rendered struct or
  method names; HIR should consume parser-owned structured metadata.

## Proof

No test run was required or performed for this inventory/probe packet.
`test_after.log` was intentionally preserved from Step 1.

Read-only evidence gathered with `c4c-clang-tool-ccdb`, `rg`, `sed`, and one
`build/c4cll --parse-only` probe of the failing fixture.
