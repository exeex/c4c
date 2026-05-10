# Sema Consteval Domain Table Authority

Status: Open
Created: 2026-05-10

Parent Ideas:
- `ideas/closed/158_sema_validate_string_authority_audit.md`
- `ideas/closed/156_parser_support_constexpr_type_helper_domain_tables.md`
- `ideas/closed/157_deferred_syntax_text_payload_contract.md`

## Goal

Move `src/frontend/sema/consteval.hpp` and
`src/frontend/sema/consteval.cpp` away from rendered string maps as ordinary
semantic authority.

Consteval evaluation may keep rendered names for diagnostics, display, legacy
no-metadata compatibility, and source syntax payloads. It should use
`TextId`, structured consteval-name keys, template binding structured keys,
and record-owner keys as the primary path whenever complete metadata is
available. A complete structured miss must not silently fall back to a rendered
`std::string` map.

## Why This Idea Exists

Idea 158 cleaned and audited `validate.cpp`, including the handoff into
consteval call environments. The next Sema layer is the consteval evaluator
itself.

Current consteval surfaces already have structured mirrors, but ordinary API
and storage still expose string-keyed maps:

- `ConstMap = std::unordered_map<std::string, long long>`
- `TypeBindings = std::unordered_map<std::string, TypeSpec>`
- `ConstEvalEnv::enum_consts`, `named_consts`, `local_consts`
- `ConstEvalEnv::enum_scopes`, `local_const_scopes`
- `ConstEvalEnv::type_bindings`
- `ConstEvalEnv::nttp_bindings`
- `ConstEvalEnv::struct_defs`
- `evaluate_consteval_call(... consteval_fns ...)`
- `bind_consteval_call_env(... out_nttp_bindings ...)`
- `InterpreterBindings::by_name`

The code already has `ConstTextMap`, `ConstStructuredMap`,
`ConstEvalFunctionTextMap`, `ConstEvalFunctionStructuredMap`,
`TypeBindingTextMap`, and `TypeBindingStructuredMap`. This idea is about
making those structured maps the contract for covered semantic lookup instead
of keeping them as secondary mirrors beside string authority.

## Responsibility Split

Parser owns syntax and carrier preservation:

- source spelling and token streams
- name `TextId`s
- template parameter order/kind metadata
- deferred syntax payloads

Sema validate owns eager declaration and visibility validation:

- it registers consteval functions, constants, scopes, and binding handoff
  metadata
- it should pass structured consteval environments to the evaluator

Sema consteval owns interpretation:

- constant lookup under local/global/enum/NTTP domains
- consteval function lookup
- local interpreter binding scopes
- template type and NTTP binding lookup during evaluation
- record-layout lookup needed for `sizeof`, `alignof`, and constant folding

HIR may provide late record/layout/template context, but Sema consteval should
consume structured handoff tables when they exist and keep rendered maps as
explicit compatibility bridges.

## In Scope

- Inventory string-keyed consteval maps and classify each as semantic
  authority, compatibility bridge, diagnostic/display, syntax payload, or
  no-metadata fallback.
- Make `ConstEvalEnv::lookup` and related value lookup paths prefer
  `ConstStructuredMap` / `ConstTextMap` over `ConstMap` when complete metadata
  is present.
- Ensure local interpreter bindings in `InterpreterBindings` use TextId and
  structured keys as authoritative before `by_name`.
- Make consteval function lookup prefer `ConstEvalFunctionStructuredMap` and
  `ConstEvalFunctionTextMap`, with rendered `consteval_fns` used only when no
  structured metadata exists.
- Make template type and NTTP bindings prefer `TypeBindingStructuredMap`,
  `TypeBindingTextMap`, `ConstStructuredMap`, and `ConstTextMap` before
  rendered parameter-name maps.
- Audit `bind_consteval_call_env` so output string maps are compatibility
  mirrors and complete metadata misses do not reopen string fallback.
- Audit consteval record-layout lookup so HIR `struct_def_owner_index` /
  record-owner keys are preferred before rendered `struct_defs`.
- Add focused tests where stale rendered consteval names, local names,
  template parameters, NTTPs, or record tags must not override structured
  metadata.
- Record any retained rendered-name map with owner, limitation, and removal
  condition.

## Out Of Scope

- Full HIR `TypeBindings` / `NttpBindings` storage migration.
- Full `src/frontend/sema/canonical_symbol.cpp` cleanup.
- Rewriting the whole consteval interpreter control-flow model.
- Removing diagnostic text, source spelling, literal payloads, or deferred
  syntax strings.
- Removing every string map in one pass when it is still needed as a
  no-metadata compatibility bridge.
- Weakening tests or marking supported consteval paths unsupported.

## Candidate Anchors

- `ConstEvalEnv::lookup` and `ConstEvalEnv::lookup_by_key` in
  `src/frontend/sema/consteval.hpp`.
- `lookup_type_binding_by_text`, `lookup_type_binding_by_key`, and
  `apply_type_bindings_to_type` in `src/frontend/sema/consteval.cpp`.
- `lookup_forwarded_nttp_arg_by_text` and NTTP binding lookup paths in
  `src/frontend/sema/consteval.cpp`.
- `bind_consteval_call_env` and its `TypeBindings` /
  `unordered_map<string, long long>` output mirrors.
- `lookup_consteval_function` and `evaluate_consteval_call`.
- `InterpreterBindings::by_name`, `by_text`, and `by_key`.
- Constant-layout record lookup through `struct_defs`,
  `struct_def_owner_index`, and `link_name_texts`.

## Acceptance Criteria

- Consteval string-keyed maps are inventoried and classified.
- Covered consteval value/function/type/NTTP/record lookup paths use
  structured keys or `TextId` maps before rendered strings when complete
  metadata exists.
- Complete structured metadata misses fail closed for covered paths instead of
  falling back to rendered string maps.
- Retained string maps are documented compatibility/no-metadata bridges with
  removal conditions.
- Focused tests prove stale rendered consteval names or binding names cannot
  override structured metadata.
- Validation covers the sema consteval tests plus any parser/HIR/frontend
  subsets touched by consteval handoff behavior.

## Reviewer Reject Signals

- A slice claims consteval authority is structured while lookup still consults
  rendered maps first.
- A change treats raw `TextId` alone as cross-domain semantic identity without
  consteval/value/template/record domain context.
- A rendered string fallback runs after a complete structured metadata miss.
- Tests only change diagnostics or expected rendered names.
- The route expands into HIR binding storage migration before Sema consteval
  authority is classified.
