Status: Active
Source Idea Path: ideas/open/165_hir_lowerer_function_context_textid_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Fence Retained String Maps and Compatibility Bridges

# Current Packet

## Just Finished

Completed plan Step 5 fencing for retained `FunctionCtx` string maps and
compatibility bridges without implementation churn.

Changed paths:
- `todo.md`

Retained string-keyed maps and bridges:

- `FunctionCtx::pack_params`
  - Owner: HIR callable parameter-pack expansion in
    `append_callable_params`/`try_expand_pack_call_arg`.
  - Limitation: keyed by the source pack parameter spelling used by the
    forwarded argument pattern. This remains string-keyed because the
    forwarding use site lacks a structured pack binding key; it only exposes
    the forwarded `NK_VAR` spelling needed to recover the emitted pack element
    params.
  - Removal condition: convert only after pack-forwarding AST/HIR carries an
    explicit structured pack binding identity from parameter declaration to
    forwarded use site.

- `FunctionCtx::label_blocks`
  - Owner: statement/control-flow lowering for `label:` and `goto label`.
  - Limitation: function-local control-flow labels are route-local block
    handles, not source value lookup authority. The map resolves goto target
    spellings to `BlockId` during lowering.
  - Removal condition: replace only if labels gain a structured function-local
    label id in the frontend/HIR contract; do not convert them to module-level
    symbols.

- Generated route-local handles in `FunctionCtx::locals`
  - Owner: lowering helpers that synthesize temporaries and slots such as
    rvalue-reference argument temps, operator/member-call temps, compound
    literal storage, and other generated local names.
  - Limitation: generated locals do not have source `TextId` identity. They are
    recorded through `rendered_compat_local_names` and use the rendered
    `locals` map as an explicit compatibility boundary.
  - Removal condition: replace only after generated HIR locals are threaded by
    `LocalId` or another structured handle at every creation/use site, with no
    parser-spelling lookup needed.

- Synthetic/no-metadata parameters in `FunctionCtx::params`
  - Owner: callable parameter lowering for generated, anonymous, and
    no-metadata parameters, including compatibility function-pointer signature
    lookup in `param_fn_ptr_sigs`.
  - Limitation: source parameters with complete `TextId` metadata use
    `param_indices_by_text_id` and `param_fn_ptr_sigs_by_index`; rendered
    `params`/`param_fn_ptr_sigs` remain only for synthetic or no-metadata
    parameter names marked by `rendered_compat_param_*`.
  - Removal condition: remove after all callable parameter producers supply
    stable source/synthetic parameter identity and no HIR lookup path requires
    rendered parameter spelling.

- Rendered compatibility mirror for source/generated locals:
  - Owner: `FunctionCtx::locals`, `local_fn_ptr_sigs`, and
    `rendered_compat_local_*`.
  - Limitation: metadata-capable source locals now use
    `local_ids_by_text_id` and `local_fn_ptr_sigs_by_id`; complete source
    misses fail closed. Rendered lookup is fenced to generated/no-metadata
    locals.
  - Removal condition: remove when generated locals and no-metadata local
    declarations have structured local identity.

- Rendered compatibility mirror for source/generated params:
  - Owner: `FunctionCtx::params`, `param_fn_ptr_sigs`, and
    `rendered_compat_param_*`.
  - Limitation: metadata-capable source params now use
    `param_indices_by_text_id` and `param_fn_ptr_sigs_by_index`; complete
    source misses do not reopen the rendered map. Rendered lookup is fenced to
    synthetic/no-metadata params.
  - Removal condition: remove when every parameter path supplies stable
    source/synthetic parameter identity.

- Rendered compatibility mirror for static globals/static locals:
  - Owner: `FunctionCtx::static_globals` plus
    `static_global_ids_by_text_id` and `rendered_compat_static_global_*`.
  - Limitation: local extern and static-local declarations with complete
    source text use `static_global_ids_by_text_id`; rendered lookup remains for
    no-metadata/generated compatibility only.
  - Removal condition: remove after static-local/global bridge users all carry
    source `TextId` or link/global identity directly.

- Rendered compatibility mirror for local consts:
  - Owner: `FunctionCtx::local_const_bindings` plus
    `local_const_bindings_by_text` and `local_const_bindings_by_key`.
  - Limitation: foldable source local const declarations now populate
    TextId/structured-key maps; complete metadata misses fail closed. The
    rendered map remains only for generated or no-metadata consteval
    compatibility.
  - Removal condition: remove after all consteval local-const producers and
    consumers carry source local-const identity or an explicit generated const
    handle.

Inventory closure: all Step 1 metadata-capable source lookup groups are now
converted or fenced. `locals`, `params`, `static_globals`,
`local_fn_ptr_sigs`, `param_fn_ptr_sigs`, and `local_const_bindings` have
structured/TextId authority for complete metadata and documented rendered
compatibility boundaries. `pack_params` and `label_blocks` are retained as
non-source-semantic string maps with explicit removal conditions.

## Suggested Next

Supervisor can move to plan Step 6 final proof and closure readiness, or send
this documentation-only Step 5 fence to reviewer if an independent drift check
is desired before final validation.

## Watchouts

- This packet intentionally did not touch implementation files. The retained
  maps were fenced by documenting current owners, limitations, and removal
  conditions only.
- `pack_params` is deliberately not converted in this plan because forwarding
  lacks a structured pack binding key at the use site.
- `label_blocks` remains a function-local control-flow map, not source value
  lookup authority.
- Rendered mirrors are compatibility boundaries for generated/no-metadata
  producers; they should not be reopened for complete source metadata misses.

## Proof

Ran delegated proof: `git diff --check -- todo.md`.

Result: passed. This documentation-only packet did not create or update
`test_after.log`; the supervisor-selected proof for this slice was a diff
whitespace check only.
