// atomic_int.h
// atomic wrapper for unsigned

/*    Copyright 2009 10gen Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#pragma once

#if defined(_WIN32)
#  include <windows.h>
#endif

namespace mongo {

    struct AtomicUInt {
        AtomicUInt() : x(0) {}
        AtomicUInt(unsigned z) : x(z) { }

        operator unsigned() const { return x; }
        unsigned get() const { return x; }

        inline AtomicUInt operator++(); // ++prefix
        inline AtomicUInt operator++(int);// postfix++
        inline AtomicUInt operator--(); // --prefix
        inline AtomicUInt operator--(int); // postfix--

        inline void zero();

        volatile unsigned x;
    };


    // this is in GCC >= 4.1
    inline void AtomicUInt::zero() { x = 0; } // TODO: this isn't thread safe - maybe
    AtomicUInt AtomicUInt::operator++() {
        return __sync_add_and_fetch(&x, 1);
    }
    AtomicUInt AtomicUInt::operator++(int) {
        return __sync_fetch_and_add(&x, 1);
    }
    AtomicUInt AtomicUInt::operator--() {
        return __sync_add_and_fetch(&x, -1);
    }
    AtomicUInt AtomicUInt::operator--(int) {
        return __sync_fetch_and_add(&x, -1);
    }

} // namespace mongo
