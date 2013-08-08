/* vim: set ts=2 sw=2 tw=99 et:
 *
 * Copyright (C) 2012 David Anderson
 *
 * This file is part of SourcePawn.
 *
 * SourcePawn is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 * 
 * SourcePawn is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * SourcePawn. If not, see http://www.gnu.org/licenses/.
 */
#ifndef _include_sourcepawn_allocatorpolicies_h_
#define _include_sourcepawn_allocatorpolicies_h_

#include <stdio.h>
#include <stdlib.h>

namespace ke {

class SystemAllocatorPolicy
{
  protected:
    void reportOutOfMemory() {
      fprintf(stderr, "OUT OF MEMORY\n");
      abort();
    }
    void reportAllocationOverflow() {
      fprintf(stderr, "OUT OF MEMORY\n");
      abort();
    }

  public:
    void free(void *memory) {
      ::free(memory);
    }
    void *malloc(size_t bytes) {
      void *ptr = ::malloc(bytes);
      if (!ptr)
        reportOutOfMemory();
      return ptr;
    }
};

}

#endif // _include_sourcepawn_allocatorpolicies_h_
