# Current Packet

Status: Active
Source Idea Path: ideas/open/127_typed_parser_record_identity_bridge.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Propagate Typed Record Identity Through Template Records

## Just Finished

Completed Step 4 first inventory for template record identity propagation.

`types/template.cpp` findings:

- `ensure_template_struct_instantiated_from_args()` already has the selected
  instantiated record `Node*` for explicit full-specialization reuse, but its
  `out_resolved` construction only fills `base` and rendered `tag`; it can
  preserve semantic identity by setting `record_def` to the selected concrete
  specialization while keeping the rendered `out_mangled` tag and map mirror.
- The injected-parse path delegates to
  `instantiate_template_struct_via_injected_parse()` and
  `parse_injected_base_type()`. This path can return a populated `TypeSpec`
  when `parse_base_type()` constructs one, but the fallback block in
  `ensure_template_struct_instantiated_from_args()` only rebuilds a tag-only
  struct `TypeSpec` from `struct_tag_def_map[*out_mangled]`. That fallback can
  preserve `record_def` from the known map node.
- Template static-member constant evaluation instantiates `Ref<Args>` and then
  immediately recovers the instantiated definition with
  `struct_tag_def_map.find(ref_mangled)`. That is a semantic lookup by rendered
  instantiation key and should become typed when
  `ensure_template_struct_instantiated_from_args()` can return `out_resolved`
  with `record_def`.
- The recursive static-member base walk still resolves `cur->base_types[bi]`
  by `base_ts.tag` through `struct_tag_def_map`. Base `TypeSpec` values can
  already carry `record_def`, so this should call
  `resolve_record_type_spec()` before compatibility fallback.
- `eval_const_int()` calls inside the same static-member path pass
  `struct_tag_def_map` as a const-eval bridge, not as direct lookup authority;
  layout consumers behind that bridge already prefer `TypeSpec::record_def`.

Adjacent template helper findings:

- `types/types_helpers.hpp::instantiate_template_struct_via_injected_parse()`
  is a token-injection bridge. It should not own semantic lookup; it can simply
  preserve whatever `parse_base_type()` returns through `out_resolved`.
- `types/base.cpp` direct template instantiation creates concrete
  `NK_STRUCT_DEF` nodes, writes `struct_tag_def_map[mangled] = inst`, and then
  currently leaves the caller `ts` tag-only. That direct producer knows the
  instantiated `Node*` and can set `ts.record_def = inst`.
- `types/base.cpp` template base propagation calls
  `ensure_template_struct_instantiated_from_args(..., &inst->base_types[bi])`,
  but both fallback blocks rebuild `inst->base_types[bi]` from `base_mangled`
  as tag-only. Those can preserve the known map node as `record_def`.
- `types/base.cpp` direct cloning of member typedefs, fields, method return
  types, and method parameter types substitutes `base`, `tag`, qualifiers, and
  pointer/ref state from bound `TypeSpec` values but does not consistently copy
  `record_def`. Any substitution from a record-backed template argument can
  lose typed identity in the cloned member/field/method payload.
- Deferred member typedef resolution in `types/base.cpp` already uses
  `lookup_struct_member_typedef_recursive_for_type()`, which was converted to
  prefer `record_def`; it mainly needs its inputs to retain `record_def`
  through the template substitution and fallback paths above.

Rendered template key classification:

- Compatibility/dedup/final-spelling: `build_template_struct_mangled_name()`,
  `make_template_struct_instance_key()`, structured/legacy instantiated-key
  synchronization, direct-emission dedup sets, `defined_struct_tags`, emitted
  tag spelling, and retained `struct_tag_def_map[mangled] = inst` mirror writes.
  These should remain rendered-key based for now.
- Semantic lookup candidates: explicit-specialization `out_resolved`, injected
  fallback `out_resolved`, template static-member owner recovery from
  `ref_mangled`, recursive static-member base lookup by `base_ts.tag`, direct
  instantiation caller `ts`, template base fallback `TypeSpec` reconstruction,
  and template type-substitution copies into member typedefs, fields, method
  returns, and method params.

## Suggested Next

First bounded implementation packet for Step 4:

- In `types/template.cpp::ensure_template_struct_instantiated_from_args()`,
  populate `out_resolved->record_def` for the explicit full-specialization
  fast path and the tag-only injected fallback when
  `struct_tag_def_map[*out_mangled]` is known.
- In `types/base.cpp`, set `ts.record_def = inst` when the direct template
  instantiation producer creates the concrete `NK_STRUCT_DEF`, and preserve
  `record_def` in the two template-base fallback reconstructions from
  `base_mangled`.
- Keep rendered instantiation keys, dedup sets, `TypeSpec::tag`, and
  `struct_tag_def_map` writes unchanged.
- Add focused parser coverage where a stale rendered template instantiation
  tag/map entry cannot override the `record_def` carried by the instantiated
  `TypeSpec`.

Suggested proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_hir_tests)$' > test_after.log 2>&1`

## Watchouts

Preserve `TypeSpec::tag` as final spelling, diagnostics, emitted text, and
compatibility payload. Do not treat `TextId` alone as semantic record identity.
Do not delete `struct_tag_def_map` while tag-only compatibility consumers
remain.

Do not convert rendered template instantiation keys used for dedup,
specialization reuse, final spelling, or direct emission. The Step 4 semantic
change is carrying `record_def` alongside those strings and making consumers
prefer it when already available.

After the first implementation packet, the next likely packet is the broader
substitution copy audit: member typedefs, fields, method returns, and method
parameters should not drop `record_def` when substituting a record-backed
template argument.

## Proof

Inventory-only packet. No build or tests were run by delegation.

Local validation: `git diff --check -- todo.md`

Proof log: no new `test_after.log` generated by this packet.
