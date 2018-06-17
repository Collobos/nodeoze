/*
 * Copyright (c) 2013-2017, Collobos Software Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _nodeoze_fs_h
#define _nodeoze_fs_h

#include <nodeoze/stream.h>
#include <nodeoze/event.h>
#include <nodeoze/filesystem.h>

namespace nodeoze {

namespace fs {

class writer : public stream::writable
{
public:

    class options
    {
    public:

        options( filesystem::path path )
        :
            m_path( std::move( path ) )
        {
        }

        const filesystem::path&
        path() const
        {
            return m_path;
        }

        inline bool
        create() const
        {
            return m_create;
        }

        inline bool
        truncate() const
        {
            return m_truncate;
        }

        inline options&
        truncate( bool val )
        {
            m_truncate = val;
            return *this;
        }

    private:

        filesystem::path    m_path;
        bool                m_create;
        bool                m_truncate;
    };

    using ptr = std::shared_ptr< writer >;

    static ptr
    create( options options );

    static ptr
    create( options options, std::error_code &err );

    virtual ~writer() = 0;
};

class reader : public stream::readable
{
public:

    class options
    {
    public:

        options( filesystem::path path )
        :
            m_path( std::move( path ) )
        {
        }

        const filesystem::path&
        path() const
        {
            return m_path;
        }

    private:

        filesystem::path m_path;
    };

    using ptr = std::shared_ptr< reader >;

    static ptr
    create( options options );

    static ptr
    create( options options, std::error_code &err );

    virtual ~reader() = 0;
};

}

}

#endif
