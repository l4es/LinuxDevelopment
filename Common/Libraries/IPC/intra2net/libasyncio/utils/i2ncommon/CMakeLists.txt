# Sources
if(NOT WITH_LIBI2NCOMMON)
    set(compat_HEADERS
        i2ncommon/containerfunc.hpp
        i2ncommon/signalfunc.hpp
        PARENT_SCOPE
    )
    set(compat_SOURCES
        i2ncommon/containerfunc.cpp
        i2ncommon/signalfunc.cpp
        PARENT_SCOPE
    )
endif(NOT WITH_LIBI2NCOMMON)
