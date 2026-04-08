#include "extern_lowering.hpp"

#include "call_decode.hpp"

namespace c4c::backend {

namespace {

struct InferredExternCallSurface {
  std::vector<std::string> param_types;
  bool is_vararg = false;
};

std::optional<InferredExternCallSurface> infer_extern_call_surface(
    const c4c::codegen::lir::LirCallOp& call) {
  InferredExternCallSurface inferred;

  if (const auto parsed = parse_backend_typed_call(call); parsed.has_value()) {
    inferred.param_types.reserve(parsed->param_types.size());
    for (const auto type : parsed->param_types) {
      inferred.param_types.push_back(std::string(type));
    }
    return inferred;
  }

  const auto parsed_vararg_call = parse_backend_direct_global_typed_call(call);
  if (!parsed_vararg_call.has_value()) {
    return std::nullopt;
  }

  inferred.is_vararg = true;
  inferred.param_types.reserve(parsed_vararg_call->typed_call.param_types.size());
  for (const auto type : parsed_vararg_call->typed_call.param_types) {
    inferred.param_types.push_back(std::string(type));
  }
  return inferred;
}

}  // namespace

std::optional<InferredExternCallSurface> infer_extern_call_surfaces(
    const c4c::codegen::lir::LirModule& module,
    const c4c::codegen::lir::LirExternDecl& decl) {
  std::optional<InferredExternCallSurface> inferred;
  const std::string callee = "@" + decl.name;

  for (const auto& function : module.functions) {
    for (const auto& block : function.blocks) {
      for (const auto& inst : block.insts) {
        const auto* call = std::get_if<c4c::codegen::lir::LirCallOp>(&inst);
        if (call == nullptr || call->callee.str() != callee) {
          continue;
        }

        const auto parsed_call = infer_extern_call_surface(*call);
        if (!parsed_call.has_value()) {
          continue;
        }

        if (!inferred.has_value()) {
          inferred = *parsed_call;
          continue;
        }
        if (inferred->param_types != parsed_call->param_types ||
            inferred->is_vararg != parsed_call->is_vararg) {
          throw LirAdapterError::unsupported(
              "minimal backend LIR adapter does not support extern declarations with inconsistent typed call surfaces");
        }
      }
    }
  }

  return inferred;
}

BackendFunction lower_extern_decl(const c4c::codegen::lir::LirModule& module,
                                  const c4c::codegen::lir::LirExternDecl& decl) {
  BackendFunction out;
  out.is_declaration = true;
  out.signature.linkage = "declare";
  out.signature.linkage_kind = BackendFunctionLinkage::Declare;
  out.signature.return_type = decl.return_type_str;
  out.signature.name = decl.name;

  if (out.signature.return_type.empty() || out.signature.name.empty()) {
    throw LirAdapterError::malformed(
        "minimal backend LIR adapter could not adapt extern declaration");
  }

  const auto inferred_surface = infer_extern_call_surfaces(module, decl);
  if (inferred_surface.has_value()) {
    out.signature.params.reserve(inferred_surface->param_types.size());
    for (const auto& type_str : inferred_surface->param_types) {
      out.signature.params.push_back(BackendParam{type_str, {}});
    }
    out.signature.is_vararg = inferred_surface->is_vararg;
  }

  return out;
}

}  // namespace c4c::backend
