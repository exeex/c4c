# Current Packet

Status: Active
Source Idea Path: ideas/open/747_frontend_source_representation_for_unresolved_external_direct_calls.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Author the required research delivery

## Just Finished

- Plan Step 1 evidence collection is complete. Candidate coverage was not
  limited to the two original observations: file-scope fixed-`void` and
  C unspecified-parameter declarations both materialize an HIR `Function`
  (the `--dump-hir` probes report `ext` through link-name, structured, and
  legacy declaration lookup); the direct-call lowerer creates a `DeclRef`
  whose `link_name_id` is that function's ID, and LIR consequently resolves
  `target_fn`. The fixed-empty facts therefore exist only on that retained
  `Function` (`params` contains the `void` sentinel for the fixed form), so
  neither is a `target_fn`-absent route.
- The additional legal alternatives also do not satisfy the four-property
  carrier: a block-scope prototype and an undeclared identifier both reach a
  plain call with no HIR function/declaration lookup hit and no `LinkNameId`;
  a function-pointer local retains its `FnPtrSig` but the callee is local and
  takes the indirect `emit_rval_id` path. A definition, redeclaration,
  qualified/overload/template direct call, or alias resolves through the same
  retained `Function`/`target_fn` mechanism; builtin alias calls instead use
  builtin canonical-name lowering with no `DeclRef` link ID or source
  fixed-empty declaration contract.
- LIR ingress confirms the split: `target.cpp` forms `@name` for any plain
  nonlocal `DeclRef`, but `find_local_target_function` sets `target_fn` when
  a declaration supplied native link identity; only the no-target branch can
  record `extern_decl_link_name_map`, and it has no `callee_fn_ptr_sig` unless
  the callee is an indirect function-pointer carrier. `structured_callee_signature`
  therefore has no native fixed-empty signature for the direct unresolved
  branch. Conclusion ready for Step 2: the currently supported source surface
  has no legal production-facing plain direct fixed-empty scalar-call carrier
  combining native global/link identity with absent `target_fn`; this is an
  evidence-bounded supported-surface absence, not an implementation change or
  an unblock/closure claim for ideas 746 or 744.

## Suggested Next

- Execute Step 2 only: author exactly the required index and numbered answer
  document from this evidence, with the candidate classification and the
  supported-surface absence conclusion; do not advance idea 746 or 744.

## Watchouts

- Idea 746 remains open and blocked. Do not treat this research activation as
  an implementation authorization or as proof that idea 746 or idea 744 is
  unblocked, complete, retired, replaced, or superseded.
- Do not change source, tests, expectations, runtime behavior, HIR, prototype
  placement, authority tables, unsupported markers, allowlists, or lifecycle
  history. Do not derive any route from text/name/type/result recovery.
- Evidence anchors for the Step 2 citations: direct-call `DeclRef` creation
  and link-ID attachment are in `src/frontend/hir/impl/expr/call.cpp`;
  function lookup/indexing is in `src/frontend/hir/hir_ir.hpp` and
  `src/frontend/hir/hir_functions.cpp`; local/parameter function-pointer
  signatures are confined to `lower_call_expr`; and LIR target/signature and
  extern-declaration handling are in `src/codegen/lir/hir_to_lir/call/target.cpp`,
  `core.cpp`, `ir.hpp`, and `hir_to_lir.cpp`.

## Proof

- `git diff --check` passed. No build or broad test run was required for this
  read-only research packet; narrow `--dump-hir` probes were run from `/tmp`
  for file-scope fixed-`void`, C unspecified, block-scope prototype,
  function-pointer, and undeclared call forms. No root proof log was created.
