//
// Copyright (C) 2011-16 DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#pragma once

#include <dynd/types/scalar_kind_type.hpp>

namespace dynd {
namespace ndt {

  class DYNDT_API int_kind_type : public base_type {
  public:
    typedef scalar_kind_type base;

    int_kind_type();

    bool match(const type &candidate_tp, std::map<std::string, type> &tp_vars) const;

    void print_data(std::ostream &o, const char *arrmeta, const char *data) const;

    void print_type(std::ostream &o) const;

    bool operator==(const base_type &rhs) const;
  };

  template <>
  struct base_of<int_kind_type> {
    typedef scalar_kind_type type;
  };

} // namespace dynd::ndt
} // namespace dynd
