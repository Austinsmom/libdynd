//
// Copyright (C) 2011-13, DyND Developers
// BSD 2-Clause License, see LICENSE.txt
//

#include <stdexcept>
#include <vector>
#include <cstdlib>

#include <dynd/memblock/objectarray_memory_block.hpp>

using namespace std;
using namespace dynd;

namespace {
    struct memory_chunk {
        char *memory;
        size_t used_count, capacity_count;
    };

    struct objectarray_memory_block {
        /** Every memory block object needs this at the front */
        memory_block_data m_mbd;
        dtype m_dt;
        const char *m_metadata;
        intptr_t m_stride;
        size_t m_total_allocated_count;
        bool m_finalized;
        /** The malloc'd memory */
        vector<memory_chunk> m_memory_handles;

        /**
         * Allocates some new memory from which to dole out
         * more. Adds it to the memory handles vector.
         */
        void append_memory(intptr_t count)
        {
            m_memory_handles.push_back(memory_chunk());
            memory_chunk& mc = m_memory_handles.back();
            mc.used_count = 0;
            mc.capacity_count = count;
            char *memory = reinterpret_cast<char *>(malloc(m_stride * count));
            mc.memory = memory;
            if (memory == NULL) {
                m_memory_handles.pop_back();
                throw bad_alloc();
            }
            m_total_allocated_count += count;
        }

        objectarray_memory_block(const dtype& dt, const char *metadata, intptr_t stride, intptr_t initial_count)
            : m_mbd(1, objectarray_memory_block_type), m_dt(dt), m_metadata(metadata),
                            m_stride(stride), m_total_allocated_count(0),
                            m_finalized(false), m_memory_handles()
        {
            if ((dt.get_flags()&dtype_flag_destructor) == 0) {
                stringstream ss;
                ss << "Cannot create objectarray memory block with dtype " << dt;
                ss << " because it does not have a destructor, use a POD memory block instead";
                throw runtime_error(ss.str());
            }
            append_memory(initial_count);
        }

        ~objectarray_memory_block()
        {
            for (size_t i = 0, i_end = m_memory_handles.size(); i != i_end; ++i) {
                memory_chunk& mc = m_memory_handles[i];
                m_dt.extended()->data_destruct_strided(m_metadata, mc.memory, m_stride, mc.used_count);
                free(mc.memory);
            }
        }
    };
} // anonymous namespace

memory_block_ptr dynd::make_objectarray_memory_block(const dtype& dt,
                const char *metadata, intptr_t stride, intptr_t initial_count)
{
    objectarray_memory_block *pmb = new objectarray_memory_block(
                    dt, metadata, stride, initial_count);
    return memory_block_ptr(reinterpret_cast<memory_block_data *>(pmb), false);
}

namespace dynd { namespace detail {

void free_objectarray_memory_block(memory_block_data *memblock)
{
    objectarray_memory_block *emb = reinterpret_cast<objectarray_memory_block *>(memblock);
    delete emb;
}

static char *allocate(memory_block_data *self, size_t count)
{
//    cout << "allocating " << size_bytes << " of memory with alignment " << alignment << endl;
    // Allocate new POD memory of the requested size and alignment
    objectarray_memory_block *emb = reinterpret_cast<objectarray_memory_block *>(self);
    memory_chunk *mc = &emb->m_memory_handles.back();
    if (mc->capacity_count - mc->used_count < count) {
        emb->append_memory(max(emb->m_total_allocated_count, count));
        mc = &emb->m_memory_handles.back();
    }

    char *result = mc->memory + emb->m_stride * mc->used_count;
    mc->used_count += count;
    if ((emb->m_dt.get_flags()&dtype_flag_zeroinit) != 0) {
        memset(result, 0, emb->m_stride * count);
    } else {
        // TODO: Add a default data constructor to base_dtype
        //       as well, with a flag for it
        stringstream ss;
        ss << "Expected objectarray data to be zeroinit, but is not with dtype " << emb->m_dt;
        throw runtime_error(ss.str());
    }
    return result;
}

static void finalize(memory_block_data *self)
{
    // Finalizes POD memory so there are no more allocations
    objectarray_memory_block *emb = reinterpret_cast<objectarray_memory_block *>(self);
    
    emb->m_finalized = true;
}

static void reset(memory_block_data *self)
{
    // Resets the POD memory so it can reuse it from the start
    objectarray_memory_block *emb = reinterpret_cast<objectarray_memory_block *>(self);
   
    if (emb->m_memory_handles.size() > 1) {
        // If there are more than one allocated memory chunks,
        // throw them all away except the last
        for (size_t i = 0, i_end = emb->m_memory_handles.size() - 1; i != i_end; ++i) {
            memory_chunk& mc = emb->m_memory_handles[i];
            emb->m_dt.extended()->data_destruct_strided(
                            emb->m_metadata, mc.memory, emb->m_stride, mc.used_count);
            free(mc.memory);
        }
        emb->m_memory_handles.front() = emb->m_memory_handles.back();
        emb->m_memory_handles.resize(1);
        // Reset to zero used elements in the chunk
        memory_chunk& mc = emb->m_memory_handles.front();
        emb->m_dt.extended()->data_destruct_strided(
                        emb->m_metadata, mc.memory, emb->m_stride, mc.used_count);
        mc.used_count = 0;
    }
}

memory_block_objectarray_allocator_api objectarray_memory_block_allocator_api = {
    &allocate,
    &finalize,
    &reset
};

}} // namespace dynd::detail

void dynd::objectarray_memory_block_debug_print(const memory_block_data *memblock, std::ostream& o, const std::string& indent)
{
    const objectarray_memory_block *emb = reinterpret_cast<const objectarray_memory_block *>(memblock);
    o << " dtype: " << emb->m_dt << "\n";
    o << " stride: " << emb->m_stride << "\n";
    if (!emb->m_finalized) {
        o << indent << " allocated count: " << emb->m_total_allocated_count << "\n";
    } else {
        o << indent << " finalized count: " << emb->m_total_allocated_count << "\n";
    } 
}