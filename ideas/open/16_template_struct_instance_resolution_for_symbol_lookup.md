# Template Struct Instance Resolution For Symbol Lookup

## Goal

Make template-struct symbol/member lookup resolve through the same
partial-specialization and instantiation machinery that already chooses
concrete struct bodies, instead of relying on half-concrete emitted-name-like
tags during later lowering.

The intended end state is:

- template-struct lookup starts from the template family plus concrete bindings,
  not from a fragile emitted-name approximation
- partial-specialization selection and template instance concretization are
  available through one shared helper path
- later consumers such as member lookup and method lookup can request a
  concrete struct instance without re-implementing specialization logic
- emitted mangled names remain a final naming surface, not the only identity
  used for semantic lookup

## Why This Is Separate

The current failure is not just an EASTL test expansion problem. It exposed a
cross-stage contract gap in how template struct instances are found.

Right now the compiler already has a partial-specialization selection path for
template structs, and it already knows how to instantiate concrete struct
bodies such as `eastl::pair<int, int>`. But later lookup surfaces still often
assume that a `TypeSpec.tag` string is already the final concrete instance tag.

That works only when the handoff is perfectly concrete. It breaks when helper
functions or deferred template paths still carry an intermediate tag such as
`eastl::pair_T1_T1_T2_T2` while codegen expects the concrete
`eastl::pair_T1_int_T2_int` layout to already exist under the same key.

This is a separate initiative because the durable requirement is broader than a
single EASTL testcase:

1. expose shared template-instance resolution as a reusable semantic service
2. keep partial-specialization selection authoritative in one place
3. stop late member/symbol lookup from depending on half-concrete tag strings
4. preserve emitted mangled names as output names rather than semantic lookup
   identities

## Scope

### 1. Shared Template Instance Resolution Helper

- extract the existing template-struct realization flow into a reusable helper
- let that helper return the selected specialization, resolved bindings, and
  concrete instance tag
- keep the helper authoritative for both primary templates and partial
  specializations

### 2. Symbol And Member Lookup Integration

- make member/symbol lookup ask the shared template-instance resolver when a
  struct tag is not already concrete
- remove the assumption that later lookup can only succeed via direct
  `struct_defs[tag]` hits on the incoming tag text
- ensure concrete struct layout lookup happens after instance resolution, not
  before

### 3. Emitted-Name Boundary Cleanup

- keep emitted names derived from the resolved concrete instance
- do not require the emitted-name format to be the semantic lookup key
- preserve the existing local mangling scheme unless a specific bug requires
  changing it

### 4. Focused Proof On The Exposed Failure

- prove that `eastl::pair<int, int>` member access no longer fails because a
  helper path still references `eastl::pair_T1_T1_T2_T2`
- use the EASTL pair/member path as the narrow proof surface for this route
- prefer semantic proof over testcase-shaped special casing

## Core Rule

Do not patch template-struct lookup by adding one-off string aliases or
testcase-shaped fallbacks for `pair`, `first`, or `second`.

The fix must make later lookup consume the same specialization/instantiation
route that already decides the concrete template struct body.

## Acceptance Criteria

- [ ] one shared helper resolves template struct instances across primary and
      partial-specialized paths
- [ ] member or symbol lookup can obtain a concrete struct instance through
      that helper instead of trusting an intermediate emitted-name-like tag
- [ ] the exposed `eastl::pair<int, int>` member-access route no longer fails
      on `pair_T1_T1_T2_T2`
- [ ] proof stays on the semantic lookup path and does not add pair-specific
      string hacks

## Non-Goals

- replacing the repo's local emitted naming with a standard C++ ABI mangler in
  the same slice
- redesigning every symbol table in the compiler before fixing the shared
  template-instance lookup seam
- broad EASTL suite expansion in the same patch

## Good First Patch

Extract a reusable template-struct instance resolver from the current
partial-specialization realization flow, hook member lookup into it before
field-resolution failure, then prove the `eastl::pair<int, int>` member path
compiles without relying on intermediate `pair_T1_T1_T2_T2` tag lookup.
