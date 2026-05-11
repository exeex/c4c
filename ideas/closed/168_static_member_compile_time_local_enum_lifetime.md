# Static Member Compile-Time Local Enum Lifetime

Status: Closed
Created: 2026-05-11
Closed: 2026-05-11

Parent Ideas:
- `ideas/closed/167_local_block_enum_scope_static_eval_structured_mirrors.md`
- `ideas/closed/164_sema_type_utils_static_eval_structured_lookup.md`

## Goal

Define and, where justified, implement scoped local/block enum lifetime for
static-member initializer evaluation and compile-time engine state without
reopening the completed lowerer-focused local/block enum route.

## Why This Idea Exists

Idea 167 completed the covered HIR lowerer paths by carrying scoped enum
`TextId` and local-key metadata into `ConstEvalEnv`. During closure, a separate
boundary remained: static-member initializer evaluation still uses its rendered
or static-member bridge, and `CompileTimeState` has no explicit local/block enum
scope lifetime model.

That residual is not a blocker for idea 167 because it is outside the completed
lowerer route and may require a distinct compile-time-engine lifetime design.
It should be handled as a separate initiative if the compiler needs local/block
enum authority in static-member or compile-time-engine contexts.

## In Scope

- Inventory static-member initializer and compile-time-engine paths that can
  observe local/block enum constants.
- Determine whether those paths need scoped local/block enum metadata, should
  remain rendered/static-member compatibility only, or are unreachable by
  well-formed local/block enum lifetimes.
- Design an explicit scoped lifetime model for any metadata-capable
  compile-time-engine path before using local/block enum `TextId` or spelling
  as authority.
- Convert only paths with honest scope lifetime metadata to structured or
  scoped TextId-aware lookup.
- Document retained compatibility bridges with owner, limitation, and removal
  condition.
- Add focused collision coverage if any static-member or compile-time-engine
  path is converted to scoped local/block enum lookup.

## Out Of Scope

- Reworking the completed idea 167 HIR lowerer scoped enum maps.
- Reopening the global/static-member structured lookup conversion completed by
  idea 164 except at the documented compatibility boundary.
- Broad consteval interpreter rewrites that are not required for local/block
  enum lifetime.
- Backend, ABI, parser, or diagnostic-string cleanup.

## Acceptance Criteria

- Static-member initializer and compile-time-engine enum lookup paths are
  inventoried and classified by local/block enum lifetime capability.
- Any converted path carries explicit scope lifetime metadata and does not use
  rendered spelling or raw `TextId` as ordinary authority.
- Complete structured misses in converted paths fail closed instead of falling
  back to rendered enum mirrors.
- Retained rendered/static-member bridges are documented as compatibility with
  a removal condition.
- Focused tests cover same-spelled local/block enum constants for each converted
  static-member or compile-time-engine path, or the inventory documents why no
  such path is currently reachable.

## Closure Summary

Completed as a documented compatibility-boundary outcome rather than a behavior
conversion.

The final inventory found no reachable static-member initializer or
compile-time-engine enum lookup path that currently carries enough scoped
local/block enum lifetime metadata to convert honestly. The lowerer-owned
`ConstEvalEnv` scoped enum stacks from idea 167 remain the only observed
scoped-lifetime capable local/block enum route.

Retained boundaries are fenced as compatibility, not local/block enum
authority:

- `StaticEvalIntEnumLookupInput` remains global/static-eval compatibility until
  a caller needing local/block enum authority routes through `ConstEvalEnv` or
  carries an explicit scoped stack.
- `CompileTimeState::enum_consts_` and `enum_consts_by_key_` remain rendered and
  global structured registries until engine work items carry explicit
  local/block enum lifetime metadata.
- `make_engine_consteval_env` remains a global registry projection into
  `ConstEvalEnv` until engine-side pending work can prove scoped lifetime
  without extending lowerer block scopes past their source lifetime.
- `eval_struct_static_member_value_hir` remains a static-member/template
  compatibility bridge until callers pass honest enum scope lifetime metadata.
- Static-member rendered maps remain compatibility mirrors; owner/member keyed
  maps are static-member authority only, not local/block enum authority.

No residual blocker remains for this idea. Any future conversion requires a new
source initiative that introduces a real scoped lifetime carrier for a reachable
static-member or compile-time-engine path.

Final proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_sema_consteval_type_utils_structured_metadata|frontend_hir_tests|frontend_hir_lookup_tests|cpp_hir_static_member_base_structured_metadata|cpp_hir_template_value_arg_static_member_trait)$' > test_after.log`

`test_after.log` reported 5/5 tests passed. The accepted full-suite baseline
remains 3135/0.

## Reviewer Reject Signals

- The route treats `CompileTimeState` as local/block enum authority without
  adding an explicit scoped lifetime model.
- A raw `TextId`, rendered name, or static-member bridge lookup is claimed as
  structured local/block enum capability without scope/domain metadata.
- The change modifies idea 167 lowerer scoped maps to force a static-member
  testcase through the completed route instead of addressing the compile-time
  boundary.
- Tests prove only one named static-member initializer case while nearby
  same-spelled local/block enum lifetime cases remain unexamined.
- Unsupported expectations are downgraded, weakened, or skipped to avoid the
  scoped lifetime problem.
- Broad consteval rewrites are introduced without showing that they are needed
  for static-member or compile-time-engine local/block enum lifetime.
