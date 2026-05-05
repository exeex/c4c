# Current Packet

Status: Active
Source Idea Path: ideas/open/144_member_template_constructor_specialization_metadata.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement Constructor Prelude Carrier

## Just Finished

Attempted Step 5's HIR callable parameter substitution repair for nested
method-template packs. The HIR-side source edits were reverted because the
live parser-to-HIR carrier is still missing the structured method-template
prelude metadata needed to bind the packs without using debug text.

Focused evidence from the reverted probe:
- Both `pair` constructor `Node`s reach HIR with `n_template_params=0`.
- The nested `tuple<Args1...>` / `tuple<Args2...>` parameter argument refs
  reach HIR as `TB_TYPEDEF` entries with invalid `template_param_text_id`,
  `template_param_index`, and `template_param_owner_text_id`.
- The `index_sequence<I1...>` / `index_sequence<I2...>` value refs reach HIR
  with invalid `nttp_text_id`; only debug/display text remains.

Because the remaining names are only debug/rendered payload, HIR cannot
semantically deduce `Args1#0=int`, `Args2#0=int`, `I1#0=0`, or `I2#0=0`
inside `lower_struct_method` / `append_callable_params` under the packet
constraints.

## Suggested Next

Implement the parser-to-HIR constructor prelude carrier seam: record-member
template constructor `Node`s must reach HIR with full structured prelude
metadata, including `n_template_params`, template parameter text ids,
type/NTTP/pack carriers, defaults where present, and nested `TemplateArgRef`
text/index/owner metadata inside constructor parameter types. Defer the HIR
callable substitution repair until that parser carrier is observable without
recovering semantics from `debug_text`.

## Watchouts

The HIR constructor registry repair still appears useful, but it is not a
commit-ready slice by itself because the delegated `^cpp_positive_sema_` proof
does not improve until the structured method-template parameter carriers are
present. No source changes are left in the tree from this blocked packet.

## Proof

Delegated proof command:
`cmake --build build && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' > test_after.log 2>&1`

Result: blocked. Existing `test_after.log` remains the refreshed delegated
proof at 883/884 passing with only
`cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp`
failing.
