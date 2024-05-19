 
#ifndef __MYOS__SAVED_H
#define __MYOS__SAVED_H

#include <common/types.h>
#include <gdt.h>

namespace myos {

struct Saved {
    common::uint32_t pid;
    common::uint32_t ppid;
    // common::uint32_t priority;
};

} // namespace myos

#endif // __MYOS__SAVED_H