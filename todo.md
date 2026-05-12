Status: Active
Source Idea Path: ideas/open/199_sema_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Fence Static-Eval Enum Compatibility

# Current Packet

## Just Finished

Step 1 is complete. Commit `acb2a34db` recorded the Sema-owned rendered
compatibility route inventory and classified static-eval enum compatibility as
the first conversion/fencing target.

The inventory covers static-eval enum helpers, rendered type identity
compatibility, consteval type-binding and NTTP bridges, rendered consteval
function lookup, interpreter/local const by-name mirrors, and
`ConstEvalEnv::struct_defs` layout handoff. It also records broad HIR lowerer
rendered maps and local-name sets as later HIR compatibility-track scope unless
a Sema proof specifically needs a boundary caller.

## Suggested Next

Execute Step 2: fence the static-eval enum compatibility route.

Owned implementation surface for the executor packet:
- `StaticEvalIntEnumLookupInput::with_rendered_enum_compatibility`
- `static_eval_int_with_rendered_enum_compatibility`
- Existing HIR/static-eval enum initializer callers needed to prove the route

Required behavior:
- Prefer `enum_consts_by_key`, `enum_consts_by_text`, structured enum identity,
  domain-scoped `TextId`, and owner-aware enum metadata where available.
- A complete structured enum key or TextId miss must fail closed and must not
  recover through stale `rendered_enum_consts`.
- Delete rendered enum recovery if no production no-metadata caller needs it.
  Otherwise retain it only as explicit legacy/no-metadata compatibility with
  owner, limitation, and removal condition in nearby comments.
- Add or tighten stale-rendered enum proof for the converted or fenced route.

## Watchouts

- Keep the work Sema-owned; do not expand into parser, broad HIR lowerer, BIR,
  LIR, backend, or full consteval-engine rewrites.
- Do not claim progress through helper renames, diagnostics-only changes, or
  expectation rewrites.
- Covered metadata-rich Sema misses must not recover through rendered enum,
  consteval function, NTTP, type-binding, local-const, or struct-def maps.
- Retained Sema bridges need `legacy` or `deprecated` comments with owner,
  limitation, and removal condition.
- Several static-eval enum paths already appear to fail closed before rendered
  lookup. Step 2 still needs stale-rendered proof and any needed comment/fence
  tightening before it can be accepted as implementation progress.

## Proof

Step 1 was inventory-only. No code validation was required by that delegated
packet, and no `test_after.log` was produced.

Step 2 requires a fresh build plus a supervisor-selected focused CTest subset
covering static-eval enum compatibility after the executor changes code or
tests.
