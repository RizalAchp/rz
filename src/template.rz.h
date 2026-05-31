#pragma once

#ifndef __RZ_H__
#    define __RZ_H__

#endif /* end of include guard: __RZ_H__ */

#ifdef RZ_IMPLEMENTATION
#    define RZ_ALLOC_IMPL
#    define RZ_ARGPARSE_IMPL
#    define RZ_COLLECTIONS_IMPL
#    define RZ_FS_IMPL
#    define RZ_LOGGER_IMPL
#    define RZ_PROCESS_IMPL
#    define RZ_SPRINTF_IMPL
#    define RZ_STRING_IMPL
#    define RZ_TESTS_IMPL
#    define RZ_TIME_IMPL
#endif

#ifdef RZ_LOGGER_IMPL
#    ifndef RZ_STRING_IMPL
#        define RZ_STRING_IMPL
#    endif
#    ifndef RZ_ALLOC_IMPL
#        define RZ_ALLOC_IMPL
#    endif
#    ifndef RZ_FS_IMPL
#        define RZ_FS_IMPL
#    endif
#endif

#ifdef RZ_TESTS_IMPL
#    ifndef RZ_ARGPARSE_IMPL
#        define RZ_ARGPARSE_IMPL
#    endif
#    ifndef RZ_TIME_IMPL
#        define RZ_TIME_IMPL
#    endif
#endif

#ifdef RZ_ARGPARSE_IMPL
#    ifndef RZ_STRING_IMPL
#        define RZ_STRING_IMPL
#    endif
#endif

#ifdef RZ_PROCESS_IMPL
#    ifndef RZ_STRING_IMPL
#        define RZ_STRING_IMPL
#    endif
#    ifndef RZ_FS_IMPL
#        define RZ_FS_IMPL
#    endif
#endif

#ifdef RZ_FS_IMPL
#    ifndef RZ_STRING_IMPL
#        define RZ_STRING_IMPL
#    endif
#    ifndef RZ_TIME_IMPL
#        define RZ_TIME_IMPL
#    endif
#endif

#ifdef RZ_TIME_IMPL
#    ifndef RZ_STRING_IMPL
#        define RZ_STRING_IMPL
#    endif
#endif

#ifdef RZ_STRING_IMPL
#    ifndef RZ_COLLECTIONS_IMPL
#        define RZ_COLLECTIONS_IMPL
#    endif
#    ifndef RZ_SPRINTF_IMPL
#        define RZ_SPRINTF_IMPL
#    endif
#endif

#ifdef RZ_COLLECTIONS_IMPL
#    ifndef RZ_ALLOC_IMPL
#        define RZ_ALLOC_IMPL
#    endif
#endif

#ifdef RZ_SPRINTF_IMPL
#    ifndef RZ_ALLOC_IMPL
#        define RZ_ALLOC_IMPL
#    endif
#endif

#ifdef RZ_ALLOC_IMPL
#endif
