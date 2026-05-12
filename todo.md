# Current Packet

Status: Active
Source Idea Path: ideas/open/172_type_identity_authority_audit.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Sema Type Authority

## Just Finished

Step 1 - Inventory Sema Type Authority completed as a code-reading audit.

Files inspected:
- `src/frontend/parser/ast.hpp`: `TypeSpec`/`TemplateArgRef` mixed carrier
  fields and inline template-dependency/debug helpers.
- `src/frontend/sema/canonical_symbol.hpp` and
  `src/frontend/sema/canonical_symbol.cpp`: `CanonicalType`,
  `CanonicalTypeIdentity`, `ResolvedTypeTable`, canonicalization,
  `types_equal`, prototype compatibility, display formatting, and ABI mangling.
- `src/frontend/sema/type_utils.hpp` and `src/frontend/sema/type_utils.cpp`:
  `TypeSpec` construction, array helpers, arithmetic classification, and
  `type_binding_values_equivalent`.
- `src/frontend/sema/consteval.hpp` and `src/frontend/sema/consteval.cpp`:
  TextId/key constant/type binding maps, rendered compatibility bridges,
  record layout lookup, and sizeof/alignof type handling.
- `src/frontend/sema/sema.cpp`, `src/frontend/sema/validate.cpp`, and
  `src/frontend/parser/parser_support.hpp`: sema pipeline entry, structured
  sema name keys, and parser-facing compatibility boundaries.

Authority classifications found:
- Syntax payload: `TypeSpec` still carries parser-produced declarator shape
  (`ptr_level`, refs, `array_rank`/`array_dims`, `inner_rank`,
  `is_ptr_to_array`, `is_fn_ptr`), qualifiers/attributes, unresolved
  `array_size_expr`, provisional `record_def`, qualifier spelling/text IDs,
  template-origin data, and deferred member-typedef payload.
- Resolved/canonical identity: `CanonicalType` and `CanonicalTypeIdentity`
  are the sema-owned structured identity surface. Identity is drawn from
  `record_def`, namespace-qualified `TextId`s, template-param owner/index IDs,
  template struct origin keys, deferred member owner/text IDs, and recursive
  pointer/array/function structure. `ResolvedTypeTable` records canonical
  types per declaration node for HIR consumption.
- TypeSpec semantic comparison: `type_binding_values_equivalent` is still a
  sema authority surface for template/type binding equivalence. It compares
  structured identity first where present (`record_def`, template-param
  metadata, qualified TextIds), then falls back through partial metadata and
  finally rendered-name compatibility for no-metadata carriers.
- Display/diagnostic use: `format_canonical_type`,
  `format_canonical_result`, template-arg debug encoders, and canonical leaf
  display spelling are render/debug paths. They should remain non-authoritative.
- ABI spelling use: `mangle_type`/`mangle_symbol` render ABI names from
  canonical type/source spelling. This is ABI output, not declaration/type
  lookup identity.
- Compatibility bridge use: rendered maps in `ConstEvalEnv`, legacy rendered
  type binding bridges, rendered NTTP bridges, parser record-tag maps, and HIR
  layout spelling recovery are explicitly compatibility paths. They generally
  gate structured TextId/key misses before falling back to rendered strings.

Highest-risk sema gaps:
- Records: sema has good structured identity in `CanonicalType` and HIR owner
  keys, but `TypeSpec` equality and consteval layout still tolerate
  no-metadata rendered record paths. `record_def` pointer identity can also be
  too local for cloned/template-instantiated records unless owner keys are
  present everywhere.
- Typedefs: `TB_TYPEDEF` resolution in consteval prefers structured key/text
  maps, but legacy display-tag bridges and final no-metadata `type_bindings`
  remain. A metadata-rich miss is mostly fail-closed, yet carriers missing
  `tag_text_id` can still depend on rendered names.
- Templates: template parameter identity is structured by owner namespace,
  owner `TextId`, index, and param `TextId`; canonical substitution still has
  a fallback name map when complete identity is absent. `TypeSpec`
  `tpl_struct_origin` falls back to string comparison if
  `tpl_struct_origin_key` is missing, and `TemplateArgRef::debug_text` remains
  a dependency/debug fallback for under-metadata paths.
- Arrays: arrays are structurally represented in both `TypeSpec` and
  `CanonicalType`, but sema has two array encodings (`array_size` and
  `array_rank`/`array_dims`) plus `array_size_expr` pointer comparison in
  `type_binding_values_equivalent`. `compute_sizeof_type` multiplies
  `array_size` then dims 1..N, so stale or partially normalized dims are a
  high-risk authority gap.
- Function pointer types: canonicalization turns `is_fn_ptr` plus
  `fn_ptr_params` into `Pointer(Function)` and `types_equal` compares the
  recursive function signature. The reverse `typespec_from_canonical` loses
  parameter list detail into `is_fn_ptr`, and raw `TypeSpec` equivalence only
  compares the `is_fn_ptr` flag/declarator fields, not the function pointer
  parameter signature.

## Suggested Next

Proceed to Step 2 by auditing HIR type refs, record/layout owner keys, pending
work maps, and any HIR rendered-name compatibility paths against the sema
authority boundaries above.

## Watchouts

- `TypeSpec` is still both syntax payload and limited semantic bridge; avoid a
  blanket rewrite. Follow-up implementation ideas should target specific
  authority gaps.
- Do not treat ABI/display/debug strings as bugs unless the string participates
  in equality, hashing, lookup, dedup, layout, or binding.
- For records, distinguish parser-local `record_def` identity from durable
  owner-key identity before proposing any layout or dedup fix.
- For function pointers, prefer canonical `Pointer(Function)` evidence over
  raw `TypeSpec::is_fn_ptr` equality when checking signature identity.

## Proof

- No tests executed; packet proof was the required read-only audit evidence in
  `todo.md`.
- AST-backed inventory used:
  `c4c-clang-tool-ccdb function-signatures` on
  `src/frontend/sema/canonical_symbol.cpp`,
  `src/frontend/sema/type_utils.cpp`, and
  `src/frontend/sema/consteval.cpp`.
