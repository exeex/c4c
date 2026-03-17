#include "hir_emitter.hpp"

namespace c4c::codegen::llvm_backend {

class HirEmitter::ConstInitEmitter {
   public:
    explicit ConstInitEmitter(HirEmitter& emitter) : emitter_(emitter) {}

    std::string emit_const_init(const TypeSpec& ts, const GlobalInit& init) {
      // Pure pointer-to-array (no outer array dims) is a scalar pointer, not an array
      if (ts.array_rank > 0 && !(ts.ptr_level > 0 && outer_array_rank(ts) == 0))
        return emit_const_array(ts, init);
      if (ts.is_vector && ts.vector_lanes > 0) return emit_const_vector(ts, init);
      if (ts.base == TB_VA_LIST && ts.ptr_level == 0) return "zeroinitializer";
      if ((ts.base == TB_STRUCT || ts.base == TB_UNION) && ts.ptr_level == 0)
        return emit_const_struct(ts, init);
      if (const auto* s = std::get_if<InitScalar>(&init))
        return emitter_.emit_const_scalar_expr(s->expr, ts);
      return (llvm_ty(ts) == "ptr") ? "null" : "zeroinitializer";
    }

    std::vector<std::string> emit_const_struct_fields(const TypeSpec& ts,
                                                      const HirStructDef& sd,
                                                      const GlobalInit& init,
                                                      std::vector<TypeSpec>* out_field_types = nullptr) {
      return emit_const_struct_fields_impl(ts, sd, init, out_field_types);
    }

   private:
    HirEmitter& emitter_;

    const Module& mod() const { return emitter_.mod_; }

    GlobalInit child_init_of(const InitListItem& item) const {
      return std::visit([&](const auto& v) -> GlobalInit {
        using V = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<V, InitScalar>) return GlobalInit(v);
        else return GlobalInit(*v);
      }, item.value);
    }

    std::string emit_const_vector(const TypeSpec& ts, const GlobalInit& init) {
      TypeSpec elem_ts = ts;
      elem_ts.is_vector = false;
      elem_ts.vector_lanes = 0;
      elem_ts.vector_bytes = 0;
      const long long n = ts.vector_lanes > 0 ? ts.vector_lanes : 1;
      std::vector<std::string> elems(static_cast<size_t>(n),
                                     emit_const_init(elem_ts, GlobalInit(std::monostate{})));
      if (const auto* scalar = std::get_if<InitScalar>(&init)) {
        if (n > 0) elems[0] = emitter_.emit_const_scalar_expr(scalar->expr, elem_ts);
      } else if (const auto* list = std::get_if<InitList>(&init)) {
        for (size_t i = 0; i < list->items.size() && i < elems.size(); ++i) {
          elems[i] = emit_const_init(elem_ts, child_init_of(list->items[i]));
        }
      }
      std::string out = "<";
      for (size_t i = 0; i < elems.size(); ++i) {
        if (i) out += ", ";
        out += llvm_ty(elem_ts) + " " + elems[i];
      }
      out += ">";
      return out;
    }

    std::string emit_const_array(const TypeSpec& ts, const GlobalInit& init) {
      const long long n = ts.array_size;
      if (n <= 0) return "zeroinitializer";
      TypeSpec elem_ts = drop_one_array_dim(ts);

      if (const auto* scalar = std::get_if<InitScalar>(&init)) {
        const Expr& e = emitter_.get_expr(scalar->expr);
        if (const auto* sl = std::get_if<StringLiteral>(&e.payload);
            sl && ts.array_rank == 1 && is_char_like(elem_ts.base) && elem_ts.ptr_level == 0) {
          std::string bytes = bytes_from_string_literal(*sl);
          if ((long long)bytes.size() > n) bytes.resize(static_cast<size_t>(n));
          if ((long long)bytes.size() < n) bytes.resize(static_cast<size_t>(n), '\0');
          return "c\"" + escape_llvm_c_bytes(bytes) + "\"";
        }
        if (const auto* sl = std::get_if<StringLiteral>(&e.payload);
            sl && sl->is_wide && ts.array_rank == 1 && elem_ts.ptr_level == 0) {
          std::vector<long long> vals = decode_wide_string_values(sl->raw);
          if ((long long)vals.size() > n) vals.resize(static_cast<size_t>(n));
          while ((long long)vals.size() < n) vals.push_back(0);
          std::string out = "[";
          const std::string elem_ty = llvm_alloca_ty(elem_ts);
          for (size_t i = 0; i < vals.size(); ++i) {
            if (i) out += ", ";
            out += elem_ty + " " + std::to_string(vals[i]);
          }
          out += "]";
          return out;
        }
        return format_array_literal(elem_ts, std::vector<std::string>(static_cast<size_t>(n), "zeroinitializer"));
      }

      std::vector<std::string> elems(static_cast<size_t>(n), "zeroinitializer");
      if (const auto* list = std::get_if<InitList>(&init)) {
        if (!is_indexed_list(*list)) return format_array_literal(elem_ts, elems);
        long long next_idx = 0;
        // When the element type is a scalar pointer (ptr-to-array with no outer dims),
        // but the init list was built for a multi-dim array, an item with bound > 1
        // and an InitList child needs to be "expanded" — each inner item maps to one
        // outer element.
        const bool elem_is_scalar_ptr = (elem_ts.ptr_level > 0 && outer_array_rank(elem_ts) == 0);
        for (const auto& item : list->items) {
          const auto maybe_idx = find_array_index(item, next_idx, n);
          if (!maybe_idx) continue;
          size_t idx = static_cast<size_t>(*maybe_idx);
          GlobalInit child = child_init_of(item);
          if (elem_is_scalar_ptr) {
            // If the child is a list, expand its items as individual scalar elements.
            // Recursively collect all scalar values from nested InitLists.
            std::function<void(const GlobalInit&, size_t&)> collect_scalars;
            collect_scalars = [&](const GlobalInit& gi, size_t& out_idx) {
              if (const auto* s = std::get_if<InitScalar>(&gi)) {
                if (out_idx < static_cast<size_t>(n))
                  elems[out_idx++] = emit_const_init(elem_ts, gi);
              } else if (const auto* cl = std::get_if<InitList>(&gi)) {
                for (const auto& inner_item : cl->items)
                  collect_scalars(child_init_of(inner_item), out_idx);
              }
            };
            if (std::holds_alternative<InitList>(child)) {
              collect_scalars(child, idx);
              next_idx = static_cast<long long>(idx);
              continue;
            }
          }
          elems[idx] = emit_const_init(elem_ts, child);
          next_idx = *maybe_idx + 1;
        }
      }
      return format_array_literal(elem_ts, elems);
    }

    std::optional<std::string> try_emit_ptr_from_char_init(const GlobalInit& init) {
      auto make_ptr_to_bytes = [&](const std::string& bytes) -> std::string {
        const std::string gname = emitter_.intern_str(bytes);
        const size_t len = bytes.size() + 1;
        return "getelementptr inbounds ([" + std::to_string(len) + " x i8], ptr " +
               gname + ", i64 0, i64 0)";
      };

      if (const auto* scalar = std::get_if<InitScalar>(&init)) {
        const Expr& e = emitter_.get_expr(scalar->expr);
        if (const auto* sl = std::get_if<StringLiteral>(&e.payload)) {
          return make_ptr_to_bytes(bytes_from_string_literal(*sl));
        }
        return std::nullopt;
      }

      const auto* list = std::get_if<InitList>(&init);
      if (!list) return std::nullopt;

      std::string bytes;
      long long next_idx = 0;
      for (const auto& item : list->items) {
        const auto maybe_idx = find_array_index(item, next_idx, -1);
        if (!maybe_idx) return std::nullopt;
        const size_t idx = static_cast<size_t>(*maybe_idx);
        if (idx > bytes.size()) bytes.resize(idx, '\0');

        GlobalInit child_init = child_init_of(item);
        const auto* child_scalar = std::get_if<InitScalar>(&child_init);
        if (!child_scalar) return std::nullopt;
        const Expr& ce = emitter_.get_expr(child_scalar->expr);
        int ch = 0;
        if (const auto* c = std::get_if<CharLiteral>(&ce.payload)) ch = c->value;
        else if (const auto* i = std::get_if<IntLiteral>(&ce.payload)) ch = static_cast<int>(i->value);
        else return std::nullopt;

        if (idx == bytes.size()) bytes.push_back(static_cast<char>(ch & 0xFF));
        else bytes[idx] = static_cast<char>(ch & 0xFF);
        next_idx = *maybe_idx + 1;
      }
      return make_ptr_to_bytes(bytes);
    }

    TypeSpec resolve_flexible_array_field_ts(const HirStructField& f,
                                            const InitListItem* item,
                                            const GlobalInit& init) {
      TypeSpec ts = emitter_.field_decl_type(f);
      if (!f.is_flexible_array) return ts;
      long long deduced = -1;
      if (item && item->resolved_array_bound) deduced = *item->resolved_array_bound;
      if (deduced <= 0) deduced = deduce_array_size_from_init(init);
      if (deduced <= 0) return ts;
      ts.array_rank = 1;
      ts.array_size = deduced;
      ts.array_dims[0] = deduced;
      return ts;
    }

    std::optional<size_t> find_union_field_index(const HirStructDef& union_sd,
                                                 const InitListItem& item) const {
      if (item.field_designator) {
        const auto fit = std::find_if(
            union_sd.fields.begin(), union_sd.fields.end(),
            [&](const HirStructField& f) { return f.name == *item.field_designator; });
        if (fit == union_sd.fields.end()) return std::nullopt;
        return static_cast<size_t>(std::distance(union_sd.fields.begin(), fit));
      }
      if (item.index_designator && *item.index_designator >= 0) {
        const size_t idx = static_cast<size_t>(*item.index_designator);
        if (idx < union_sd.fields.size()) return idx;
      }
      return std::nullopt;
    }

    std::optional<long long> find_array_index(const InitListItem& item,
                                              long long next_idx,
                                              long long bound) const {
      long long idx = next_idx;
      if (item.index_designator && *item.index_designator >= 0) idx = *item.index_designator;
      if (idx < 0) return std::nullopt;
      if (bound >= 0 && idx >= bound) return std::nullopt;
      return idx;
    }

    bool is_explicitly_mapped_item(const InitListItem& item) const {
      return item.field_designator.has_value() || item.index_designator.has_value();
    }

    bool is_explicitly_mapped_list(const InitList& list) const {
      return std::all_of(list.items.begin(), list.items.end(), [&](const InitListItem& item) {
        return is_explicitly_mapped_item(item);
      });
    }

    bool is_indexed_list(const InitList& list) const {
      return std::all_of(list.items.begin(), list.items.end(), [](const InitListItem& item) {
        return item.index_designator.has_value() && *item.index_designator >= 0;
      });
    }

    std::optional<size_t> find_struct_field_index(const HirStructDef& sd,
                                                  const InitListItem& item,
                                                  size_t next_idx) const {
      size_t idx = next_idx;
      if (item.field_designator) {
        const auto fit = std::find_if(
            sd.fields.begin(), sd.fields.end(),
            [&](const HirStructField& f) { return f.name == *item.field_designator; });
        if (fit == sd.fields.end()) return std::nullopt;
        idx = static_cast<size_t>(std::distance(sd.fields.begin(), fit));
      } else if (item.index_designator && *item.index_designator >= 0) {
        idx = static_cast<size_t>(*item.index_designator);
      }
      if (idx >= sd.fields.size()) return std::nullopt;
      return idx;
    }

    std::optional<std::pair<size_t, GlobalInit>> try_select_canonical_union_field_init(
        const HirStructDef& union_sd, const GlobalInit& union_init) const {
      if (const auto* list = std::get_if<InitList>(&union_init)) {
        if (list->items.empty()) return std::nullopt;
        const auto& item0 = list->items.front();
        const auto maybe_idx = find_union_field_index(union_sd, item0);
        if (maybe_idx) return std::pair<size_t, GlobalInit>(*maybe_idx, child_init_of(item0));
      }
      return std::nullopt;
    }

    std::vector<std::string> emit_const_struct_fields_impl(const TypeSpec&,
                                                           const HirStructDef& sd,
                                                           const GlobalInit& init,
                                                           std::vector<TypeSpec>* out_field_types) {
      std::vector<TypeSpec> field_types;
      field_types.reserve(sd.fields.size());
      for (const auto& f : sd.fields) field_types.push_back(emitter_.field_decl_type(f));

      std::vector<std::string> field_vals;
      field_vals.reserve(sd.fields.size());
      for (size_t i = 0; i < sd.fields.size(); ++i) {
        field_vals.push_back(emit_const_init(field_types[i], GlobalInit(std::monostate{})));
      }
      auto update_field_type =
          [&](size_t idx, const InitListItem* item, const GlobalInit& child_init) -> const TypeSpec& {
        if (idx + 1 == sd.fields.size() && sd.fields[idx].is_flexible_array) {
          field_types[idx] = resolve_flexible_array_field_ts(sd.fields[idx], item, child_init);
        }
        return field_types[idx];
      };
      auto emit_field_mapped_item = [&](size_t idx, const InitListItem& item) {
        GlobalInit child_init = child_init_of(item);
        const TypeSpec& field_ts = update_field_type(idx, &item, child_init);
        if (llvm_field_ty(sd.fields[idx]) == "ptr") {
          if (auto ptr_init = try_emit_ptr_from_char_init(child_init)) {
            field_vals[idx] = *ptr_init;
            return;
          }
        }
        field_vals[idx] = emit_const_init(field_ts, child_init);
      };

      if (const auto* list = std::get_if<InitList>(&init)) {
        if (!is_explicitly_mapped_list(*list)) {
          if (out_field_types) *out_field_types = std::move(field_types);
          return field_vals;
        }
        size_t next_idx = 0;
        for (const auto& item : list->items) {
          const auto maybe_idx = find_struct_field_index(sd, item, next_idx);
          if (!maybe_idx) continue;
          const size_t idx = *maybe_idx;
          emit_field_mapped_item(idx, item);
          next_idx = idx + 1;
        }
      }

      if (out_field_types) *out_field_types = std::move(field_types);
      return field_vals;
    }

    std::string emit_const_struct(const TypeSpec& ts, const GlobalInit& init) {
      if (!ts.tag || !ts.tag[0]) return "zeroinitializer";
      const auto it = mod().struct_defs.find(ts.tag);
      if (it == mod().struct_defs.end()) return "zeroinitializer";
      const HirStructDef& sd = it->second;
      if (sd.is_union) return emit_const_union(ts, sd, init);
      return format_struct_literal(sd, emit_const_struct_fields_impl(ts, sd, init, nullptr));
    }

    std::string emit_const_union(const TypeSpec& ts, const HirStructDef& sd, const GlobalInit& init) {
      if (sd.fields.empty()) return "zeroinitializer";
      const int sz = sd.size_bytes;
      auto zero_union = [&]() -> std::string {
        if (sz == 0) return "zeroinitializer";
        return "{ [" + std::to_string(sz) + " x i8] zeroinitializer }";
      };
      auto copy_bytes = [](std::vector<unsigned char>& dst, size_t dst_off,
                           const std::vector<unsigned char>& src, size_t max_copy) -> void {
        if (dst_off >= dst.size() || src.empty() || max_copy == 0) return;
        const size_t copy_n = std::min({dst.size() - dst_off, src.size(), max_copy});
        if (copy_n == 0) return;
        std::memcpy(dst.data() + dst_off, src.data(), copy_n);
      };

      std::function<bool(const TypeSpec&, const GlobalInit&, std::vector<unsigned char>&)> encode_bytes;
      encode_bytes = [&](const TypeSpec& cur_ts, const GlobalInit& cur_init,
                         std::vector<unsigned char>& out) -> bool {
        const int cur_sz = std::max(1, sizeof_ts(mod(), cur_ts));
        out.assign(static_cast<size_t>(cur_sz), 0);
        if (cur_ts.ptr_level > 0 || cur_ts.is_fn_ptr) return false;
        const bool is_aggregate_target =
            cur_ts.array_rank > 0 ||
            ((cur_ts.base == TB_STRUCT || cur_ts.base == TB_UNION) && cur_ts.ptr_level == 0);
        if (!is_aggregate_target && std::holds_alternative<InitList>(cur_init)) return true;

        if (cur_ts.array_rank > 0) {
          const long long n = cur_ts.array_size;
          if (n <= 0) return true;
          TypeSpec elem_ts = drop_one_array_dim(cur_ts);
          const int elem_sz = std::max(1, sizeof_ts(mod(), elem_ts));
          if (const auto* scalar = std::get_if<InitScalar>(&cur_init)) {
            const Expr& e = emitter_.get_expr(scalar->expr);
            if (const auto* sl = std::get_if<StringLiteral>(&e.payload)) {
              if (elem_ts.ptr_level == 0 && is_char_like(elem_ts.base)) {
                const std::string bytes = bytes_from_string_literal(*sl);
                const size_t max_n = static_cast<size_t>(n);
                for (size_t i = 0; i < max_n && i < bytes.size(); ++i) {
                  out[i * static_cast<size_t>(elem_sz)] = static_cast<unsigned char>(bytes[i]);
                }
                if (bytes.size() < max_n) {
                  out[bytes.size() * static_cast<size_t>(elem_sz)] = 0;
                }
                return true;
              }
            }
            return true;
          }
          if (const auto* list = std::get_if<InitList>(&cur_init)) {
            if (!is_indexed_list(*list)) return true;
            long long next_idx = 0;
            for (const auto& item : list->items) {
              const auto maybe_idx = find_array_index(item, next_idx, n);
              if (!maybe_idx) continue;
              std::vector<unsigned char> elem;
              if (encode_bytes(elem_ts, child_init_of(item), elem)) {
                const size_t off = static_cast<size_t>(*maybe_idx) * static_cast<size_t>(elem_sz);
                copy_bytes(out, off, elem, static_cast<size_t>(elem_sz));
              }
              next_idx = *maybe_idx + 1;
            }
            return true;
          }
          return true;
        }

        if ((cur_ts.base == TB_STRUCT || cur_ts.base == TB_UNION) && cur_ts.tag && cur_ts.tag[0]) {
          const auto sit = mod().struct_defs.find(cur_ts.tag);
          if (sit == mod().struct_defs.end()) return false;
          const HirStructDef& cur_sd = sit->second;
          if (cur_sd.is_union) {
            if (const auto canonical = try_select_canonical_union_field_init(cur_sd, cur_init)) {
              if (canonical->first >= cur_sd.fields.size()) return true;
              std::vector<unsigned char> fb;
              if (!encode_bytes(emitter_.field_decl_type(cur_sd.fields[canonical->first]), canonical->second, fb))
                return false;
              copy_bytes(out, 0, fb, out.size());
              return true;
            }
            return true;
          }

          if (const auto* list = std::get_if<InitList>(&cur_init)) {
            if (!is_explicitly_mapped_list(*list)) return true;
            size_t next_field = 0;
            for (const auto& item : list->items) {
              const auto maybe_field = find_struct_field_index(cur_sd, item, next_field);
              if (!maybe_field) continue;
              const size_t fi = *maybe_field;
              std::vector<unsigned char> fb;
              if (encode_bytes(emitter_.field_decl_type(cur_sd.fields[fi]), child_init_of(item), fb)) {
                const size_t off = static_cast<size_t>(cur_sd.fields[fi].offset_bytes);
                copy_bytes(out, off, fb, out.size() - off);
              }
              next_field = fi + 1;
            }
            return true;
          }
          return true;
        }

        if (const auto* scalar = std::get_if<InitScalar>(&cur_init)) {
          const Expr& e = emitter_.get_expr(scalar->expr);
          unsigned long long v = 0;
          if (const auto* il = std::get_if<IntLiteral>(&e.payload)) v = static_cast<unsigned long long>(il->value);
          else if (const auto* cl = std::get_if<CharLiteral>(&e.payload)) v = static_cast<unsigned long long>(cl->value);
          else return false;
          for (int i = 0; i < cur_sz; ++i) {
            out[static_cast<size_t>(i)] = static_cast<unsigned char>((v >> (8 * i)) & 0xFF);
          }
          return true;
        }
        return true;
      };

      auto emit_union_from_field =
          [&](size_t field_idx, const GlobalInit& child_init) -> std::optional<std::string> {
        if (field_idx >= sd.fields.size()) return std::nullopt;
        std::vector<unsigned char> bytes(static_cast<size_t>(sz), 0);
        std::vector<unsigned char> field_bytes;
        if (!encode_bytes(emitter_.field_decl_type(sd.fields[field_idx]), child_init, field_bytes)) {
          return std::nullopt;
        }
        if (field_bytes.empty()) return std::nullopt;
        copy_bytes(bytes, 0, field_bytes, bytes.size());
        const bool all_zero = std::all_of(bytes.begin(), bytes.end(), [](unsigned char b) { return b == 0; });
        if (all_zero) return zero_union();
        std::string arr = "[";
        for (size_t i = 0; i < bytes.size(); ++i) {
          if (i) arr += ", ";
          arr += "i8 " + std::to_string(static_cast<unsigned int>(bytes[i]));
        }
        arr += "]";
        return "{ [" + std::to_string(sz) + " x i8] " + arr + " }";
      };

      if (const auto selected = try_select_canonical_union_field_init(sd, init)) {
        if (auto out = emit_union_from_field(selected->first, selected->second)) return *out;
      }
      return zero_union();
    }

    std::string format_array_literal(const TypeSpec& elem_ts, const std::vector<std::string>& elems) const {
      std::string out = "[";
      const std::string elem_ty = llvm_alloca_ty(elem_ts);
      // For scalar element types, zeroinitializer is invalid — use "0" or "0.0".
      // Complex types lower to { iN, iN } structs, so they are not scalar.
      const bool elem_is_scalar = elem_ts.array_rank == 0 && elem_ts.ptr_level == 0 &&
                                  !elem_ts.is_fn_ptr && elem_ts.base != TB_STRUCT &&
                                  elem_ts.base != TB_UNION && elem_ts.base != TB_VA_LIST &&
                                  !elem_ts.is_vector && !is_complex_base(elem_ts.base);
      const std::string zero_val = elem_is_scalar
          ? (is_float_base(elem_ts.base) ? "0.0" : "0")
          : "zeroinitializer";
      for (size_t i = 0; i < elems.size(); ++i) {
        if (i) out += ", ";
        const std::string& val = (elems[i] == "zeroinitializer" && elem_is_scalar) ? zero_val : elems[i];
        out += elem_ty + " " + val;
      }
      out += "]";
      return out;
    }

    std::string format_struct_literal(const HirStructDef& sd,
                                      const std::vector<std::string>& field_vals) const {
      std::string out = "{ ";
      bool first = true;
      int cur_offset = 0;
      int last_idx = -1;
      for (size_t i = 0; i < sd.fields.size();) {
        const auto& field = sd.fields[i];
        if (field.llvm_idx == last_idx) {
          ++i;
          continue;
        }
        last_idx = field.llvm_idx;
        if (field.offset_bytes > cur_offset) {
          if (!first) out += ", ";
          first = false;
          out += "[" + std::to_string(field.offset_bytes - cur_offset) +
                 " x i8] zeroinitializer";
          cur_offset = field.offset_bytes;
        }
        if (!first) out += ", ";
        first = false;
        out += llvm_field_ty(field) + " ";
        if (field.bit_width < 0) {
          out += field_vals[i];
          cur_offset = field.offset_bytes + std::max(0, field.size_bytes);
          ++i;
          continue;
        }
        unsigned long long packed = 0;
        int storage_size = std::max(0, field.size_bytes);
        while (i < sd.fields.size() && sd.fields[i].llvm_idx == last_idx) {
          const auto& bf = sd.fields[i];
          if (bf.bit_width > 0) {
            long long val = 0;
            try { val = std::stoll(field_vals[i]); } catch (...) {}
            const unsigned long long mask =
                (bf.bit_width >= 64) ? ~0ULL : ((1ULL << bf.bit_width) - 1);
            packed |= (static_cast<unsigned long long>(val) & mask) << bf.bit_offset;
          }
          storage_size = std::max(storage_size, bf.size_bytes);
          ++i;
        }
        out += std::to_string(static_cast<long long>(packed));
        cur_offset = field.offset_bytes + storage_size;
      }
      if (sd.size_bytes > cur_offset) {
        if (!first) out += ", ";
        out += "[" + std::to_string(sd.size_bytes - cur_offset) + " x i8] zeroinitializer";
      }
      out += " }";
      return out;
    }

    long long deduce_array_size_from_init(const GlobalInit& init) const {
      if (const auto* list = std::get_if<InitList>(&init)) {
        long long max_idx = -1;
        long long next = 0;
        for (const auto& item : list->items) {
          long long idx = next;
          if (item.index_designator && *item.index_designator >= 0) idx = *item.index_designator;
          if (idx > max_idx) max_idx = idx;
          next = idx + 1;
        }
        return max_idx + 1;
      }
      if (const auto* scalar = std::get_if<InitScalar>(&init)) {
        const Expr& e = emitter_.get_expr(scalar->expr);
        if (const auto* sl = std::get_if<StringLiteral>(&e.payload)) {
          return static_cast<long long>(bytes_from_string_literal(*sl).size()) + 1;
        }
      }
      return -1;
    }

  };

std::vector<std::string> HirEmitter::emit_const_struct_fields(const TypeSpec& ts,
                                                              const HirStructDef& sd,
                                                              const GlobalInit& init,
                                                              std::vector<TypeSpec>* out_field_types) {
  return ConstInitEmitter(*this).emit_const_struct_fields(ts, sd, init, out_field_types);
}

std::string HirEmitter::emit_const_init(const TypeSpec& ts, const GlobalInit& init) {
  return ConstInitEmitter(*this).emit_const_init(ts, init);
}

}  // namespace tinyc2ll::codegen::llvm_backend
