# Current Packet

Status: Active
Source Idea Path: ideas/open/144_member_template_constructor_specialization_metadata.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement Method Type-Pack Deduction

## Just Finished

Implemented Step 4's parser-to-HIR constructor prelude carrier. Record-member
template constructor `Node`s now receive the member-template prelude metadata
that was previously only transient parser scope:

- `n_template_params` and parallel template parameter arrays are attached to
  the constructor/method `Node`.
- Template parameter text ids, type-vs-NTTP flags, pack flags, and simple
  default carriers are preserved.
- Nested `TemplateArgRef` type args inside constructor parameter types are
  annotated with template-param text/index/owner metadata.
- Forwarded NTTP pack args such as `Is...` are accepted as value template args
  and keep their parser-owned `nttp_text_id`.

The focused metadata proof covers a generic record-member template constructor
with a defaulted type parameter, a type pack, and an NTTP pack; it verifies the
constructor `Node` and nested `TemplateArgRef` carriers directly, without
recovering semantics from debug/rendered strings.

## Suggested Next

Implement Step 5 HIR constructor registration/specialization now that the
parser carrier is observable. The next slice should consume the structured
method-template type-pack and NTTP-pack metadata on constructor `Node`s,
specialize the constructor overload, and realize nested parameter types such as
`Owner<Pack...>` from structured pack bindings.

## Watchouts

The delegated positive-Sema proof is still 883/884 because the HIR constructor
registry/specialization seam is not implemented in this slice. The remaining
failure is still:

`cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp`

The previous HIR workaround should not recover from `debug_text`; it can now
read the constructor `Node` prelude metadata and nested `TemplateArgRef`
identity directly.

## Proof

Focused metadata proof:
`cmake --build build && ctest --test-dir build --output-on-failure -R '^cpp_hir_parser_declarations_residual_structured_metadata$'`

Result: passed, 1/1.

Delegated positive-Sema proof:
`cmake --build build && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' > test_after.log 2>&1`

Result: unchanged and monotonic, 883/884 passing with only
`cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp`
failing. Proof log: `test_after.log`.
