# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.4.4.1
Current Step Title: Re-Audit Member-Typedef Mirror Writers And Readers

## Just Finished

Step 2.4.4.1 re-audited the live rendered `owner::member`
member-typedef mirror writers/readers after Step 2.4.3. This was a
lifecycle/code-reading inventory only; no implementation files or tests were
changed.

Live rendered mirror writers/readers found:

- `src/frontend/parser/impl/core.cpp:1059`
  `Parser::register_struct_member_typedef_binding(owner_name, member_name,
  type)` is still a rendered writer API. It builds `struct_typedefs` by
  reparsing `owner_name` text through `make_struct_member_typedef_key`, then
  also writes a rendered `owner::member` spelling through
  `register_typedef_binding`. Strongest real carrier at the API boundary:
  missing metadata/rendered owner plus member text. The only live caller found
  is the template-instantiation writer in `types/base.cpp`.
- `src/frontend/parser/impl/types/struct.cpp:2310`
  `register_record_member_typedef_bindings` is the record-body writer. It now
  has direct record/member metadata plus namespace context and `TextId`
  carriers (`sd`, `sd->namespace_context_id`, `sd->unqualified_text_id`,
  member text id) and writes the structured direct key with
  `record_member_typedef_key_in_context`. It still also writes a rendered
  `source_tag::member` fallback through `register_typedef_binding` when
  `source_tag` is present. Strongest real carrier: direct record/member
  metadata plus namespace context and `TextId`.
- `src/frontend/parser/impl/types/base.cpp:3223`
  the template-instantiation writer clones member typedef arrays onto the
  instantiated record and calls
  `register_struct_member_typedef_binding(mangled, member_name, member_ts)`.
  It has template-instantiation metadata nearby
  (`structured_emitted_instance_key`, primary-template key, concrete argument
  keys, `tpl_def`, `inst`, and member typedef arrays), but the instantiated
  `Node` is named only by rendered `mangled` text and does not carry a
  non-rendered concrete owner key usable as a member-typedef owner. Strongest
  real carrier: template-instantiation metadata, with a concrete owner-key
  metadata blocker.
- `src/frontend/parser/impl/types/declarator.cpp:637`
  the dependent-template-owner path writes `cache_typedef_type(spelled_name,
  owner_ts)` for spellings such as `Template<Args>::member` after preserving
  `tpl_struct_origin` plus `deferred_member_type_name` in `TypeSpec`.
  Strongest real carrier: template-instantiation metadata on the `TypeSpec`;
  storage key remains rendered spelling.
- `src/frontend/parser/impl/types/declarator.cpp:789`
  the nested/direct owner path writes `cache_typedef_type(dep_name,
  resolved_member)` after resolving the owner through structured record
  authority and scanning direct `owner->member_typedef_*` arrays. Strongest
  real carrier: direct record/member metadata. The rendered cache is a
  compatibility write after direct resolution, not the source of authority.
- `src/frontend/parser/impl/core.cpp:690` and `:704`
  `has_typedef_type(TextId)` and `find_typedef_type(TextId)` are still generic
  rendered qualified-spelling readers. For a `TextId` whose text contains
  `::`, they reconstruct a `QualifiedNameKey` with `known_fn_name_key(0,
  name_text_id)`, check `struct_typedefs`, then fall back to
  `non_atom_typedef_types` for non-symbol names. Strongest real carrier:
  `TextId` plus rendered qualified spelling; callers with a real
  `QualifiedNameKey` should use `find_typedef_type(const QualifiedNameKey&)`.
- `src/frontend/parser/impl/types/base.cpp:772`, `:1443`, and `:1799`
  are parse-base/type-argument readers that still call
  `find_typedef_type(find_parser_text_id(rendered_ref))` for qualified strings
  after or around more structured paths. Strongest real carrier varies by
  caller: direct record/member metadata is available in
  `lookup_struct_member_typedef_recursive_for_type`, but `ref`/`tname` string
  paths remain rendered `TextId` readers.
- `src/frontend/parser/impl/types/declarator.cpp:590` and `:648`
  are dependent-typename reader probes that still ask
  `has_typedef_type(find_parser_text_id(resolved))` before falling through to
  structured owner traversal. Strongest real carrier after the miss: direct
  record/member metadata. The rendered probe is still live compatibility
  behavior.
- `src/frontend/parser/impl/expressions.cpp:1394`
  is no longer a mirror-only reader for member typedef recognition when
  structured record authority exists: it first resolves the owner via
  `qualified_type_record_definition_from_structured_authority` and scans
  `owner->member_typedef_*`. Its later parse probe can still observe the
  generic rendered typedef reader indirectly. Strongest real carrier: direct
  record/member metadata, with rendered reader exposure only through the
  fallback parse probe.

Smallest safe next structural conversion/blocker: Step 2.4.4.2 should focus on
the template-instantiation writer. Either add/pass a caller-owned structured
concrete-instantiation owner carrier from the existing
`structured_emitted_instance_key`/template argument metadata into member
typedef registration, or stop on the concrete metadata blocker. Wrapper/API
deletion, helper renaming, local splitting of rendered `mangled`, or local
splitting of rendered `owner::member` text is explicitly rejected as progress.

## Suggested Next

Next bounded packet: execute Step 2.4.4.2 against
`src/frontend/parser/impl/types/base.cpp` and the minimal parser metadata
surface it needs. Determine whether the concrete instantiated record can carry
or expose a structured owner key derived from
`structured_emitted_instance_key`; if yes, register member typedefs through
that carrier. If not, record the missing concrete-instantiation owner metadata
as the blocker instead of reconstructing identity from `mangled`.

## Watchouts

Do not delete `register_struct_member_typedef_binding` or the rendered
compatibility mirror while `types/base.cpp:3223` still depends on rendered
`mangled` as semantic owner identity.

Do not count splitting `mangled`, `source_tag::member`, `spelled_name`, or
`dep_name` into owner/member pieces as structural progress. The accepted route
must start from direct `Node` member arrays, `QualifiedNameKey`, namespace
context plus `TextId`, or explicit template-instantiation metadata.

The record-body writer already has enough direct record/member metadata for a
future cleanup, but converting it first would leave the harder
template-instantiation semantic writer untouched. The direct record-body route
should remain secondary until the template-instantiation blocker is resolved or
formally parked.

## Proof

Lifecycle/code-reading inventory only. No code tests were delegated or run, and
no new code proof is claimed. `test_after.log` was intentionally left
unchanged.
