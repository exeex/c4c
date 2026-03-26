# Runtime Template Call Bring-up Plan

## Purpose

This document is for an execution-oriented AI agent.

Read it as a runbook.
Do not redesign the whole frontend in one pass.
Do not merge parse, sema, and codegen failures into one vague "template bug".
Do not start in unrelated STL/EASTL mega-cases.


## One-Sentence Goal

Fix the three reduced positive runtime regressions that currently block the next
EASTL/libstdc++ bring-up step:

- braced temporary as a templated call argument
- template-id temporary followed by `operator()`
- dependent member call with forwarded parameter-pack arguments

while keeping each fix attached to the smallest responsible layer.


## Core Rule

Treat these as three different failure classes:

1. parse of braced temporary arguments in templated calls
2. lowering/codegen of template-id temporary call expressions
3. call argument collection for dependent member calls with pack expansion

But do **not** build three unrelated pipelines.

Instead:

- keep one expression / call parsing pipeline
- keep one lowering path for call expressions
- isolate fixes by the first reduced failing shape


## Read First

Before editing, open these exact files:

1. [tests/cpp/internal/postive_case/tag_brace_forward_runtime.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/tag_brace_forward_runtime.cpp)
2. [tests/cpp/internal/postive_case/template_temporary_call_runtime.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/template_temporary_call_runtime.cpp)
3. [tests/cpp/internal/postive_case/member_call_forward_pack_runtime.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/member_call_forward_pack_runtime.cpp)
4. [tests/cpp/internal/postive_case/sizeof_pack_template_arg_parse.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/sizeof_pack_template_arg_parse.cpp)
5. [src/frontend/parser/expressions.cpp](/workspaces/c4c/src/frontend/parser/expressions.cpp)
6. [src/frontend/parser/types.cpp](/workspaces/c4c/src/frontend/parser/types.cpp)

Only open codegen after you prove the parser is producing the intended call
shape for the temporary-call case.

Do not begin in:

- unrelated EASTL headers
- HIR files
- generic STL frontier notes
- broad refactors of all pack-expansion logic


## Current Acceptance Targets

Keep these tests in mind throughout the work:

- [tag_brace_forward_runtime.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/tag_brace_forward_runtime.cpp)
- [template_temporary_call_runtime.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/template_temporary_call_runtime.cpp)
- [member_call_forward_pack_runtime.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/member_call_forward_pack_runtime.cpp)
- [sizeof_pack_template_arg_parse.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/sizeof_pack_template_arg_parse.cpp)
- [eastl_vector_simple.cpp](/workspaces/c4c/tests/cpp/eastl/eastl_vector_simple.cpp)


## Non-Goals

Do not do these as part of this run:

- promise full `eastl::vector` support in one patch
- redesign the entire call-expression AST
- rewrite all pack-expansion parsing
- fix every remaining libstdc++ tuple/invoke error before the reduced cases pass


## Working Model

You are building toward this staged frontend model:

- parser accepts:
  - `Tag{}`
  - `Cmp<2>()(a, b)`
  - `alloc.construct(&out, forward<Args>(args)...)`
- AST/HIR preserve the intended call structure
- codegen can lower temporary-call expressions without treating the callee as an lvalue slot

You are building toward this bring-up model:

- reduced runtime tests fail first
- reduced runtime tests go green one by one
- only then reprobe `eastl_vector_simple`


## Execution Rules

Follow these rules exactly:

1. One edit slice should target one failure class.
2. Always keep the reduced positive test failing or passing for an understood reason.
3. Do not "fix" `eastl::vector` directly while any reduced reproducer is still red.
4. Use parse-only / dump-first checks before touching lowering for a runtime failure.
5. After each step, rerun only the smallest relevant test first.
6. Only after the reduced tests pass, reprobe `eastl_vector_simple`.


## Step 1. Freeze the Runtime Regression Surface

Goal:

- know exactly which reduced runtime failures are the current guardrails

Do this:

1. open [tests/cpp/internal/InternalTests.cmake](/workspaces/c4c/tests/cpp/internal/InternalTests.cmake)
2. verify all four acceptance-target tests are registered
3. confirm these three are runtime-mode tests:
   - [tag_brace_forward_runtime.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/tag_brace_forward_runtime.cpp)
   - [template_temporary_call_runtime.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/template_temporary_call_runtime.cpp)
   - [member_call_forward_pack_runtime.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/member_call_forward_pack_runtime.cpp)
4. confirm [sizeof_pack_template_arg_parse.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/sizeof_pack_template_arg_parse.cpp) stays in parse-only guardrails

Completion check:

- the reduced runtime frontier is explicit and registered


## Step 2. Fix Braced Temporary Arguments In Templated Calls

Goal:

- make `impl<R>(Tag{}, forward<A>(a))` parse successfully

Primary target:

- [tag_brace_forward_runtime.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/tag_brace_forward_runtime.cpp)

Current failure shape:

- `expected RPAREN but got '{'`

Do this:

1. inspect call-argument parsing in [expressions.cpp](/workspaces/c4c/src/frontend/parser/expressions.cpp)
2. localize where `Tag{}` is being rejected in argument position
3. add the narrowest parser support needed for braced temporary / braced-init argument expressions
4. do not broaden initializer-list semantics beyond what this case needs

Completion check:

- `tag_brace_forward_runtime` passes


## Step 3. Fix Template-Id Temporary Call Lowering

Goal:

- make `Cmp<2>()(a, b)` compile and run correctly

Primary target:

- [template_temporary_call_runtime.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/template_temporary_call_runtime.cpp)

Current failure shape:

- `StmtEmitter: cannot take lval of expr`

Do this:

1. first verify the parsed call structure using parse-only / dump output
2. only after the parser shape is confirmed, inspect the lowering/codegen path that handles call callees
3. fix the smallest lowering assumption that incorrectly treats the temporary callee as an lvalue storage location
4. do not rewrite all functor/lambda call lowering in the same step

Completion check:

- `template_temporary_call_runtime` passes end-to-end


## Step 4. Fix Dependent Member Calls With Forwarded Packs

Goal:

- make `alloc.construct(&out, forward<Args>(args)...)` compile and run correctly

Primary target:

- [member_call_forward_pack_runtime.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/member_call_forward_pack_runtime.cpp)

Current failure shape:

- `function call arity mismatch`

Do this:

1. inspect argument collection around member-call parsing / call lowering
2. localize whether the pack-expanded forwarded arguments are dropped in parse, sema, or lowering
3. fix only the path needed for:
   - dependent member callee
   - one ordinary leading argument
   - forwarded pack tail
4. do not mix in unrelated variadic-template rewrites

Completion check:

- `member_call_forward_pack_runtime` passes end-to-end


## Step 5. Keep `sizeof...(T)` As A Guardrail

Goal:

- ensure the earlier `sizeof...(T)` fix does not regress while fixing the call frontier

Primary target:

- [sizeof_pack_template_arg_parse.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/sizeof_pack_template_arg_parse.cpp)

Do this:

1. rerun this parse-only test after every call-related parser edit
2. if it regresses, reduce the interaction before continuing

Completion check:

- `sizeof_pack_template_arg_parse` stays green throughout the run


## Step 6. Reprobe `eastl_vector_simple`

Goal:

- measure the real frontier after the reduced call/runtime fixes land

Primary target:

- [eastl_vector_simple.cpp](/workspaces/c4c/tests/cpp/eastl/eastl_vector_simple.cpp)

Do this:

1. run it with the same EASTL/EABase include paths used today
2. record the new first failing header/location
3. only then choose the next reduced repro

Completion check:

- you know whether the frontier moved and what the next concrete blocker is


## Suggested Test Order

Use this order after each relevant edit:

1. the single reduced test for the slice you touched
2. [sizeof_pack_template_arg_parse.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/sizeof_pack_template_arg_parse.cpp)
3. the other reduced runtime tests
4. [eastl_vector_simple.cpp](/workspaces/c4c/tests/cpp/eastl/eastl_vector_simple.cpp) parse-only probe


## What “Done” Means For This Plan

This execution plan is complete when all of the following are true:

1. [tag_brace_forward_runtime.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/tag_brace_forward_runtime.cpp) passes
2. [template_temporary_call_runtime.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/template_temporary_call_runtime.cpp) passes
3. [member_call_forward_pack_runtime.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/member_call_forward_pack_runtime.cpp) passes
4. [sizeof_pack_template_arg_parse.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/sizeof_pack_template_arg_parse.cpp) remains green
5. `eastl_vector_simple` is reprobed and its new first blocker is documented or improved


## If You Get Stuck

Do this:

1. stop broad refactoring
2. confirm the current reduced test still fails for the expected reason
3. dump the parsed shape before touching codegen
4. make one smaller fix
5. rerun only the nearest relevant tests

Do not do this:

- do not hide parser bugs with codegen heuristics
- do not patch EASTL headers
- do not jump back to `std::vector` while the reduced runtime tests are still red
