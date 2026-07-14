# Legal Source-to-HIR Representation for Unresolved External Direct Calls

## Question

Can a legal production-facing source form produce a plain direct fixed-empty
scalar call with native global/link identity while LIR call-target ingress has
no `target_fn`?

## Evidence boundary and trace

This answer covers the supported parser/sema/HIR forms relevant to a direct
call rather than inferring an absence from two fixtures. The narrow HIR probes
used C source equivalents of the forms below. They covered a file-scope
fixed-`void` declaration, a C unspecified-parameter declaration, a block-scope
prototype, a function-pointer local, and an undeclared identifier.

The frontend has one decisive declaration path:

1. `Lowerer::lower_function` creates an HIR `Function`, interns its
   `link_name_id`, records its parameters, and marks a bodyless declaration
   `extern` in `src/frontend/hir/hir_functions.cpp`.
2. Direct-call construction in `src/frontend/hir/impl/expr/call.cpp` creates a
   `DeclRef` and obtains its carrier `LinkNameId` by resolving that function.
   The ordinary fallback attaches the same ID to a plain nonlocal `DeclRef`.
3. `Module::resolve_function_decl` and `resolve_direct_call_callee` in
   `src/frontend/hir/hir_ir.hpp` resolve the `DeclRef` through link-name,
   structured declaration, or the bounded legacy compatibility lookup.
4. `StmtEmitter::resolve_call_target_info` in
   `src/codegen/lir/hir_to_lir/call/target.cpp` starts a nonlocal `DeclRef` as
   a global spelling, then calls `find_local_target_function`. A retained HIR
   function sets `target_fn`, replaces the emitted name with the function link
   name, and supplies return/signature facts from that function.

For a no-target plain `DeclRef`, the same LIR ingress can form a rendered
`@name` and call `record_extern_call_decl`, but this is not native declaration
authority. It records the no-target result in
`extern_decl_link_name_map` only when a `LinkNameId` is already present;
otherwise it uses the legacy name map (`src/codegen/lir/ir.hpp`).
`structured_callee_signature` in `target.cpp` produces a signature only from
`target_fn` or `callee_fn_ptr_sig`. A plain unresolved direct `DeclRef` has
neither. The later `extern_decls` vector is only the finalized declaration
receipt (`src/codegen/lir/hir_to_lir/hir_to_lir.cpp`), not a source-signature
carrier.

For a fixed empty C prototype, the native fact is the retained HIR
`Function::params` single `void` sentinel: `function_has_void_param_list` and
`lir_call_signature_from_function` convert it to `has_void_param_list` with
no fixed parameters. C `f()` instead retains the `Function` with
`attrs.unspecified_params`; it is not a fixed-empty declaration.

## Candidate classification

| Legal source family | Declaration retention and `DeclRef` | `target_fn` at LIR ingress | Native direct global/link identity | Fixed-empty signature facts | Production carrier result |
| --- | --- | --- | --- | --- | --- |
| File-scope `extern double ext(void); … ext();` | HIR dump reports `fn ext(<anon_param>: void) -> double extern` and declaration lookup via link-name, structured, and legacy routes. The direct-call `DeclRef` receives the function link ID. | Present: `find_local_target_function` resolves the retained `Function`. | Present, but it is the same function identity that resolves `target_fn`. | Present on `Function::params` as the `void` sentinel. | Not compliant: the required absence of `target_fn` is impossible on this carrier. |
| File-scope C `extern double ext(); … ext();` | HIR dump likewise reports retained `fn ext() -> double extern` and the same `DeclRef` lookup routes. | Present. | Present only through the retained function. | Not fixed-empty: C `()` is retained as `unspecified_params`. | Not compliant: `target_fn` is present and the declaration is not fixed-empty. |
| File-scope declaration without `extern`, repeated declarations, or a definition | All are function declarations processed by `lower_function`; the definition adds a body but retains the same callable identity. Qualified, overload-resolved, template-instantiated, and method direct-call forms construct a resolved direct-call `DeclRef` and use the corresponding retained `Function`. | Present for the resolved callable. | Present through that `Function`. | Available when the retained declaration uses `(void)`. | Not compliant: all direct declaration variants take the `target_fn` route. |
| Block-scope prototype, `double probe(void) { extern double ext(void); return ext(); }` | The probe HIR contains only `probe`, an empty declaration statement, and `return ext()`; there is no `ext` function or declaration lookup hit. The plain call's `DeclRef` therefore has no function-derived link ID. | Absent. | Absent as native HIR `LinkNameId`; a rendered `@ext` is only the unresolved fallback. | No retained callable signature exists. | Not compliant: it reaches no-target handling but lacks native direct identity and fixed-empty authority. |
| Undeclared direct identifier, `double probe(void) { return ext(); }` | The probe HIR contains only `probe` and no `ext` lookup hit or retained declaration. | Absent. | Absent as native HIR `LinkNameId`; only rendered unresolved spelling remains. | No declaration or `FnPtrSig` exists. | Not compliant: this is the original no-target observation, without the required facts. |
| Local, parameter, static-global, or global function-pointer call | `lower_call_expr` retains `FnPtrSig` only for local/parameter/static-global pointer carriers. The callee `DeclRef` is local, parameter, or global rather than a plain nonlocal direct function reference. | No direct function target is required. | Not a direct global function call: `resolve_call_target_info` uses `emit_rval_id` for those carriers. | May be present in `FnPtrSig`, including an empty `void` list. | Not compliant: signature authority belongs to an indirect call, not the required plain direct global carrier. |
| Casted/address-taken function or other expression-shaped callee | The underlying declared function is retained, but the call callee is no longer the plain nonlocal `DeclRef` route; expression emission determines the callee value. | It cannot establish the requested no-target plain-direct route. | No independent native direct-call carrier is added. | A declaration may have facts, but they remain associated with its retained `Function` or pointer expression. | Not compliant: changing expression shape does not combine the required direct and no-target properties. |
| Builtin alias call (for example `__builtin_*` aliases) | The builtin table identifies `BuiltinCategory::AliasCall` and canonical name; this is builtin lowering, not a source declaration `DeclRef` carrier. | Absent as a source function target. | Canonical rendered name can be emitted, but `callee_link_name_id` is not obtained from a source declaration. | No source fixed-empty declaration contract; aliases in the table are variadic or otherwise unrelated call families. | Not compliant and outside the plain source-declaration route. |

## Result

No currently supported legal source form combines all four required properties:

1. plain direct scalar call;
2. native global/link identity;
3. `target_fn` absent at LIR ingress; and
4. native fixed-empty declaration facts.

The retained-function route supplies properties 1, 2, and—when declared as
`(void)`—4, but necessarily supplies `target_fn`. The unresolved/block-scope
route can leave `target_fn` absent, but supplies neither a native `LinkNameId`
nor a retained fixed-empty signature. Function-pointer forms can retain a
`FnPtrSig`, but are indirect and therefore not a substitute. Builtin aliases
are a separate builtin mechanism, not a production-facing source-declaration
carrier.

Accordingly, this is a supported-surface absence conclusion. It does not
authorize HIR fabrication, prototype relocation, parallel authority storage,
or recovery from text, names, types, rendered results, or printer output. It
also does not itself unblock, complete, supersede, retire, or close ideas 746
or 744; the evidence must be considered in a separate lifecycle decision for
idea 746.
