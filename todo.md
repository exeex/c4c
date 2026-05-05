# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Delete TypeSpec Tag And Validate

## Just Finished

Step 6's final constructor packet is blocked before a commit-ready repair.

The first bad boundary is the parser/HIR carrier for member-template
constructors. The constructor overload is now registered for
`eastl::pair_T1_int_T2_int`, but the member constructor `Node` does not carry
its own `template<class... Args1, ...>` parameter list. The remaining NTTP pack
use in `index_sequence<I1...>` also arrives without a structured
`nttp_text_id`, so the private delegating constructor cannot instantiate
`get<I1>(first_args)` with a concrete tuple owner. HIR still lowers `get` with
`t: struct<?>*&`, and LIR fails on `.value`.

## Suggested Next

Next packet should repair parser-to-HIR member-template constructor metadata:
carry method template type-pack and NTTP-pack parameter lists, including
structured text ids for pack references inside constructor initializer
arguments.

## Watchouts

- Do not reintroduce `TypeSpec::tag` or rendered-string semantic lookup.
- A partial HIR-side deduction attempt can specialize the public `pair<int,int>`
  constructor, but it cannot safely recover `I1`/`I2` without the missing
  structured NTTP carrier. Do not replace that with debug-text or rendered-name
  recovery.

## Proof

Delegated proof command:
`cmake --build build && ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_' > test_after.log 2>&1`

Result: blocked. The full delegated proof was refreshed in `test_after.log`
and remained at 883/884 passing with only:
- `cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp`

The packet is not commit-ready for code. Canonical proof log:
`test_after.log`.
