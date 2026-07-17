# Project-Wide C++20 Host Toolchain Contract

Status: Closed (capability complete)
Type: project-wide build/toolchain migration

## Closure Record

Idea 802 is capability-complete. The project now has one required,
non-extension C++20 host-build authority, configure-time capability checks,
and a durable host-toolchain contract. All 139 generated compile-command
entries use `-std=c++20`, with no remaining in-scope C++17 or GNU-extension
host mode. The bounded NodeKind authoring surface now uses named designated
schema rows, typed stage/arity/refinement policies, per-entry `consteval`
construction, and retained registry-wide validation without changing the
pass-facing helper API or established semantics.

Accepted implementation and evidence commits are `7175a52fe`, `4d523ddec`,
`4b4df45ec`, and `74fde86ae`. The supervisor-owned matching configure, build,
and CTest evidence records 1272 passed and the same 40 pre-existing failures
out of 1312 tests both before and after; the failure sets are identical, the
monotonic regression guard passes, and no new test exceeds 30 seconds. Focused
backend proof passes 6/6. No target or test was disabled, no language-under-test
fixture contract changed, and idea 732 was not modified.

## Goal

Migrate the entire c4c implementation, its production targets, and its native
test executables from the C++17 host-language standard to one consistently
declared C++20 standard. Publish a supported compiler and standard-library
floor, reject unsupported configurations at configure time, and prove that a
clean supported-host build preserves the existing test contract.

This idea changes the language standard used to build c4c itself. It does not
change which C or C++ language revisions c4cll accepts, rejects, preprocesses,
or diagnoses as the compiler under test.

## Why This Exists

The repository currently pins C++17 in several independent places. Current
evidence includes the root target/profile and the frontend, codegen, and
backend production targets, together with native preprocessor, frontend,
backend BIR, and backend MIR tests. The declarations mix
`target_compile_features(... cxx_std_17)` with target-local
`CXX_STANDARD 17`, making drift and partial migration likely.

C++20 is useful as a common implementation baseline for future work, but the
migration must first establish a truthful repository-wide toolchain contract.
The one explicitly approved language-driven convergence is the NodeKind
registry authoring surface: it should become visibly and locally C++20 while
preserving the pass-facing helper API and the single-registry semantics from
idea 801. Other facilities must not be opportunistically rewritten to
concepts, ranges, `consteval`, or other C++20 designs. CMake currently declares
a minimum version of 3.20; the migration must remain compatible with that
declared CMake floor unless changing it is separately justified and approved.

## In Scope

### 1. Complete standard-declaration audit

- Audit all versioned and generated build inputs for hard-coded
  `cxx_std_17`, `CXX_STANDARD 17`, `CMAKE_CXX_STANDARD 17`, manual
  `-std=c++17`/equivalent flags, and duplicated or indirectly inherited C++
  standard declarations.
- Classify each occurrence as production code, native host-side test/tooling,
  third-party/external code, or c4cll language-under-test input.
- Record the authoritative declaration point and remove redundant local pins
  where inheritance is intentional and verifiable.

### 2. One project-wide C++20 authority

- Establish one reviewed CMake authority for the host implementation standard
  and require C++20 without compiler extensions unless an existing target has
  an explicitly documented exception.
- Raise every production and native test target that independently owns a
  standard declaration to that shared C++20 contract.
- Ensure newly added in-tree C++ targets inherit or consume the same authority
  rather than silently defaulting to a compiler-specific language mode.
- Preserve deliberate third-party target boundaries; do not rewrite external
  projects merely to make their declarations textually identical.

### 3. Supported compiler and standard-library floor

- Define the minimum supported compiler versions and corresponding standard
  libraries for every supported host toolchain family actually claimed by the
  project.
- Base the floor on the C++20 facilities c4c requires at migration time, not on
  an aspirational list of unused features.
- Add configure-time checks that fail early with an actionable diagnostic for
  an unsupported compiler, compiler mode, or standard-library capability.
- Keep compiler identity and standard-library identity distinct where their
  compatibility can vary independently.

### 4. Clean configuration and build proof

- Configure from a clean build directory using each available supported host
  toolchain selected for acceptance.
- Prove the effective compile mode is C++20 for representative production and
  native test targets, rather than relying only on the source CMake text.
- Build all normal in-tree production targets and native test targets enabled
  by the accepted presets/options.
- Treat warnings, compatibility failures, and dependency conflicts as evidence
  to resolve or explicitly bound; do not hide them by selectively disabling
  affected targets.

### 5. Regression proof

- Capture matching before/after CTest baselines using the same configuration,
  enabled options, environment, and test selection.
- Run the full relevant CTest surface for the supported host configuration,
  plus any narrower configure/build probes needed to demonstrate toolchain
  fail-fast behavior.
- Permit only evidence-backed pre-existing host/platform failures; no new
  failures, skipped supported coverage, weakened expectations, or relabeled
  unsupported behavior may be attributed to the standard migration.

### 6. Durable toolchain documentation

- Document the C++20 host-language requirement, supported compiler and
  standard-library floor, CMake requirement, extension policy, and the
  configure-time rejection behavior.
- Clearly distinguish host implementation C++20 from the source-language modes
  exercised by c4cll tests.
- Document where the central standard authority lives and how a future target
  must consume it.

### 7. Bounded NodeKind C++20 authoring convergence

- Preserve the existing pass-facing compile-time and runtime helper API and
  the single authoritative NodeKind registry semantics established by idea
  801.
- Replace the registry's long positional construction with an agent-readable
  named schema construction using C++20 designated initialization or an
  equally explicit named builder.
- Introduce typed stage, arity, and refinement policies where they prevent
  accidental mixing of masks, counts, or unrelated classification axes.
- Use `consteval` or an equivalent forced compile-time local construction path
  so invalid individual entries fail near their declaration with actionable
  diagnostics, while retaining registry-wide completeness, uniqueness, and
  relational validation.
- Keep every current NodeKind represented exactly once and preserve its
  accepted classification, payload contract, stage admission, and SSA query
  semantics unless an existing defect is independently demonstrated.
- Make the C++20 dependency obvious at the registry authoring site so a future
  agent reading or extending the code can recognize the project standard and
  follow the named schema pattern.

## Language-Under-Test Boundary

Test fixtures containing C++20 syntax may be input programs for c4cll's
preprocessor, parser, semantic analysis, or code-generation tests. Their source
language flags and expected diagnostics describe compiler-under-test behavior;
they are not evidence of, and must not be mechanically rewritten as, the host
standard used to compile the test harness.

The audit must therefore distinguish:

- C++ sources compiled into c4c or a native test executable, which move to the
  host C++20 contract; and
- fixture/input sources parsed or compiled by c4cll, whose requested language
  mode remains governed by that test's semantic purpose.

## Out of Scope

- Opportunistic rewrites to concepts, ranges, `consteval`, `std::span`, class
  non-type template parameters, modules, coroutines, or other C++20 APIs
  outside the bounded NodeKind authoring convergence explicitly in scope.
- Redesigning the NodeKind pass-facing helper API, single-registry authority,
  tag algebra semantics, BIR storage, or phase vocabulary established by ideas
  746 and 801.
- Replacing the NodeKind registry with a typelist hierarchy, concept-heavy DSL,
  class-type-NTTP metaprogramming language, or another abstraction whose
  diagnostics and maintenance burden exceed the named schema it replaces.
- Revising, activating, or executing idea 732.
- Upgrading third-party dependencies unless a demonstrated C++20 compatibility
  failure makes a narrowly scoped change unavoidable and separately reviewed.
- Changing c4cll's supported source-language standards, feature gates,
  diagnostics, or frontend semantics.
- Broad warning cleanup, formatting churn, unrelated CMake modernization, or a
  general minimum-CMake-version migration.
- Disabling production targets or tests to obtain a green C++20 build.

## Open Design Choices Requiring Approval

Before activation, the execution plan must make these choices explicit:

1. Which host compiler families and platforms are support claims rather than
   best-effort configurations.
2. The exact minimum compiler and standard-library versions, justified by
   clean probes on available toolchains and CI policy.
3. Whether the central authority is a repository interface target, a top-level
   compile-feature policy, or another CMake-3.20-compatible mechanism. The
   result must still avoid duplicate per-target version declarations.
4. Whether compiler-version checks alone are sufficient for any family or a
   small compile-capability probe is required for the standard library.

## Acceptance Criteria

- A repository-wide audit accounts for every `cxx_std_17`, `CXX_STANDARD 17`,
  `CMAKE_CXX_STANDARD 17`, equivalent manual flag, and duplicated standard
  declaration.
- Every in-scope production and native test target compiles under required,
  non-extension C++20 through one documented CMake authority or an explicitly
  reviewed exception.
- No in-scope target silently remains C++17 or compiler-default, and generated
  compile commands demonstrate the effective C++20 mode.
- The supported compiler and standard-library floor is documented and enforced
  by configure-time fail-fast checks with actionable messages.
- Clean configure and complete build succeed on every host toolchain selected
  for acceptance.
- Matching before/after full relevant CTest logs show no new failures, no
  reduced pass count caused by disabled coverage, and no unsupported
  expectation downgrade.
- c4cll language-under-test fixtures retain their intended language modes and
  semantic expectations; the migration changes only the native host build
  unless a fixture itself is proven to be a host source.
- The durable toolchain documentation identifies the C++20 authority, compiler
  and library floor, CMake floor, extension policy, and future-target usage.
- The NodeKind registry uses an agent-readable named C++20 authoring form,
  typed stage/arity/refinement policies where they prevent category mixing,
  local forced compile-time validation with actionable failure locality, and
  registry-wide completeness/uniqueness/relational checks.
- Existing NodeKind pass-facing helpers, single-registry ownership, current
  kind classifications, payload behavior, stage admission, and SSA semantics
  remain compatible and covered by focused compile-time/runtime tests.
- The accepted diff contains no opportunistic C++20 refactor, NodeKind semantic
  redesign, idea-732 work, or unrelated dependency upgrade.

## Reviewer Reject Signals

- Reject a textual `17`-to-`20` replacement that leaves multiple competing
  standard authorities or fails to prove the effective compiler mode.
- Reject a migration that updates only the root or production libraries while
  native preprocessor, frontend, BIR, MIR, codegen, or other in-tree test/tool
  targets remain on C++17 or compiler-default mode.
- Reject treating c4cll input fixtures containing C++17/C++20 constructs as
  host implementation sources, or rewriting their source-language flags and
  expectations without a separate semantic requirement.
- Reject compiler-version claims without a corresponding standard-library
  contract when the selected host can pair the compiler with incompatible
  library versions.
- Reject unsupported-toolchain behavior that reaches opaque compilation errors
  instead of the required configure-time diagnostic.
- Reject disabling targets, narrowing CTest selection, lowering expectations,
  marking supported behavior unsupported, or accepting new failures to make
  the C++20 migration appear green.
- Reject named-test shortcuts, expectation-only changes, diagnostic
  reclassification, or helper renames claimed as migration capability.
- Reject a NodeKind change that merely renames the positional builder, creates
  a second registry, duplicates compile/runtime facts, weakens completeness
  checks, or changes the pass-facing helper API without an independently
  demonstrated requirement.
- Reject typelist/concept DSL overengineering, opaque template diagnostics, or
  class-type-NTTP machinery that makes adding one NodeKind harder to understand
  than filling a named schema.
- Reject opportunistic concepts/ranges/`consteval` refactors outside the
  bounded NodeKind authoring surface, NodeKind semantic or BIR redesign,
  idea-732 changes, broad CMake modernization, or unrelated dependency
  upgrades in the migration slice.
- Reject a new policy abstraction that still permits target-local standard
  drift or retains the old duplicated C++17/C++20 declaration failure mode
  behind a different CMake helper name.
