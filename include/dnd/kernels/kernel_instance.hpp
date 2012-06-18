//
// Copyright (C) 2011-12, Dynamic NDArray Developers
// BSD 2-Clause License, see LICENSE.txt
//

#ifndef _DND__KERNEL_INSTANCE_HPP_
#define _DND__KERNEL_INSTANCE_HPP_

#include <dnd/auxiliary_data.hpp>

namespace dnd {

typedef void (*nullary_operation_t)(char *dst, intptr_t dst_stride,
                        intptr_t count, const AuxDataBase *auxdata);

typedef void (*unary_operation_t)(char *dst, intptr_t dst_stride,
                        const char *src0, intptr_t src0_stride,
                        intptr_t count, const AuxDataBase *auxdata);

typedef void (*binary_operation_t)(char *dst, intptr_t dst_stride,
                        const char *src0, intptr_t src0_stride,
                        const char *src1, intptr_t src1_stride,
                        intptr_t count, const AuxDataBase *auxdata);

/**
 * Unary operations support four specializations as a standardized
 * set, in the order defined by unary_specialization_t. Use the
 * function get_unary_specialization to get an index into an
 * instance of this table.
 */
typedef unary_operation_t specialized_unary_operation_table_t[4];

enum unary_specialization_t {
    // Arbitrary strides
    general_unary_specialization,
    // Both src and dst have stride of zero (always only one element)
    scalar_unary_specialization,
    // Both src and dst are contiguous
    contiguous_unary_specialization,
    // The src stride is zero, the dst stride is contiguous
    scalar_to_contiguous_unary_specialization
};

// Given strides and element sizes, returns the appropriate unary_specialization enum value.
inline unary_specialization_t get_unary_specialization(intptr_t dst_stride, intptr_t src_stride,
                                intptr_t src_element_size, intptr_t dst_element_size)
{
    // The idea of this expression is to have no branches, just a deterministic calculation
    return static_cast<unary_specialization_t>(
                (((dst_stride == dst_element_size)& // dst is contiguous
                  ((src_stride == 0)|               // src is scalar
                   (src_stride == src_element_size) // src is contiguous
                 )) << 1
                ) |
                (src_stride == 0));                 // src is scalar
}

/**
 * This class holds an instance of a kernel function, with its
 * associated auxiliary data. The object is non-copyable, just
 * like the auxiliary_data object, to avoid inefficient copies.
 */
template<typename FT>
class kernel_instance {
    kernel_instance& operator=(const kernel_instance&);
public:
    kernel_instance()
        : kernel(0)
    {
    }
    // Copying a kernel_instance clones the auxiliary data
    kernel_instance(const kernel_instance& rhs)
        : kernel(rhs.kernel)
    {
        rhs.auxdata.clone_into(auxdata);
    }

    void swap(kernel_instance& rhs) {
        std::swap(kernel, rhs.kernel);
        auxdata.swap(rhs.auxdata);
    }

    FT kernel;
    auxiliary_data auxdata;
};

} // namespace dnd;



#endif // _DND__KERNEL_INSTANCE_HPP_
