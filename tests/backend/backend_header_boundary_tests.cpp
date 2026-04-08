#include "backend.hpp"

#include <utility>
#include <type_traits>

template <typename T, typename = void>
struct IsCompleteType : std::false_type {};

template <typename T>
struct IsCompleteType<T, std::void_t<decltype(sizeof(T))>> : std::true_type {};

#include "../../src/backend/x86/codegen/emit.hpp"

static_assert(!IsCompleteType<c4c::codegen::lir::LirModule>::value,
              "x86 emit header should not complete the frontend LIR module just to expose emitter entrypoints");

#include "../../src/backend/aarch64/codegen/emit.hpp"

static_assert(!IsCompleteType<c4c::codegen::lir::LirModule>::value,
              "aarch64 emit header should not complete the frontend LIR module just to expose emitter entrypoints");

#include "../../src/backend/bir.hpp"

static_assert(std::is_constructible_v<c4c::backend::BackendModuleInput,
                                      const c4c::backend::bir::Module&>,
              "backend input surface should accept a direct BIR module");
static_assert(std::is_constructible_v<c4c::backend::BackendModuleInput,
                                      const c4c::codegen::lir::LirModule&>,
              "backend input surface should accept a direct LIR entry module");
static_assert(!std::is_constructible_v<c4c::backend::BackendModuleInput,
                                       const c4c::backend::bir::Module*>,
              "backend input surface should not advertise nullable BIR pointer state");
static_assert(!std::is_constructible_v<c4c::backend::BackendModuleInput,
                                       const c4c::codegen::lir::LirModule*>,
              "backend input surface should not advertise nullable LIR pointer state");
static_assert(!std::is_constructible_v<c4c::backend::BackendModuleInput,
                                       const c4c::backend::bir::Module&,
                                       const c4c::codegen::lir::LirModule*>,
              "backend input surface should not advertise a dead prelowered-BIR legacy fallback pointer");
