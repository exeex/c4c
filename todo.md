# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Delete TypeSpec Tag And Validate

## Just Finished

Step 6's parser-to-HIR member-template constructor packet is blocked before a
commit-ready repair.

The exact carrier boundary is now narrower: the record-member template prelude
can be attached to constructor `Node`s, and the generated specialization
`eastl::pair_T1_T_T2_T` has a structured `struct_def_nodes_` entry that points
back to the primary `eastl::pair` with both constructors. However,
instantiated/delegating constructor lowering still has no coherent method
template specialization carrier. A local experiment that projected the prelude
and supplemented constructor lookup reached HIR, but the public and private
constructor bodies still lowered with unspecialized `Args1`/`Args2` and
`I1`/`I2`: `get` instantiated as `eastl::get<I=?,T=?>`, `first_args` stayed
`struct eastl::tuple_T_T`, and `auto p = eastl::pair<int,int>(...)` still
deduced as `void`. That non-commit-ready experiment was reverted.

## Suggested Next

Next packet should implement a coherent constructor method-template
specialization path: carry record-member template prelude metadata onto
constructor `Node`s, deduce method type packs from constructor call/delegating
arguments, bind NTTP packs from `index_sequence<I...>`, and lower the selected
constructor specialization under the instantiated owner.

## Watchouts

- Do not reintroduce `TypeSpec::tag` or rendered-string semantic lookup.
- The generated specialization tag is not itself enough. The structured node
  map can find the primary private constructor, but using that constructor
  without method-template pack bindings leaves `get<I1>(first_args)` unknown.
- Do not replace the missing `I1`/`I2` pack binding with debug-text,
  rendered-name, `_t`, or `tag_ctx` recovery.

## Proof

Delegated proof command:
`cmake --build build && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' > test_after.log 2>&1`

Result: blocked. Existing full delegated proof in `test_after.log` remains at
883/884 passing with only:
- `cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp`

No source implementation was left dirty from the failed experiment, and the
packet is not commit-ready for code. Canonical proof log: `test_after.log`.
