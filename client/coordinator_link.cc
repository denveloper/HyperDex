// Copyright (c) 2012, Cornell University
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of HyperDex nor the names of its contributors may be
//       used to endorse or promote products derived from this software without
//       specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

// POSIX
#include <poll.h>

// HyperDex
#include "common/configuration.h"
#include "client/coordinator_link.h"

using hyperdex::configuration;
using hyperdex::coordinator_link;

coordinator_link :: coordinator_link(const po6::net::hostname& host)
    : m_config()
    , m_repl(host.address.c_str(), host.port)
    , m_get_config_id(-1)
    , m_get_config_status(REPLICANT_GARBAGE)
    , m_get_config_output(NULL)
    , m_get_config_output_sz(0)
{
}

coordinator_link :: ~coordinator_link() throw ()
{
}

const configuration&
coordinator_link :: config()
{
    return m_config;
}

// wait_for_config returns true iff there is a new config and returns false
// otherwise, specifying why the error occurred.
bool
coordinator_link :: wait_for_config(hyperclient_returncode* status)
{
    uint64_t version = m_config.version();

    if (!poll_for_config(status))
    {
        return false;
    }

    while (version >= m_config.version())
    {
        pollfd pfd;
        pfd.fd = m_repl.poll_fd();
        pfd.events = POLLIN;
        pfd.revents = 0;

        if (poll(&pfd, 1, -1) <= 0)
        {
            *status = HYPERCLIENT_POLLFAILED;
            return false;
        }

        if (!poll_for_config(status))
        {
            return false;
        }
    }

    return true;
}

bool
coordinator_link :: poll_for_config(hyperclient_returncode* status)
{
    // if we need to issue a "get-config" request
    if (m_get_config_id < 0)
    {
        m_get_config_id = m_repl.send("hyperdex", "get-config", "", 0,
                                      &m_get_config_status,
                                      &m_get_config_output,
                                      &m_get_config_output_sz);

        if (m_get_config_id < 0)
        {
            switch (m_get_config_status)
            {
                case REPLICANT_SERVER_ERROR:
                case REPLICANT_NEED_BOOTSTRAP:
                case REPLICANT_MISBEHAVING_SERVER:
                    *status = HYPERCLIENT_COORDFAIL;
                    return false;
                case REPLICANT_SUCCESS:
                case REPLICANT_NAME_TOO_LONG:
                case REPLICANT_FUNC_NOT_FOUND:
                case REPLICANT_OBJ_EXIST:
                case REPLICANT_OBJ_NOT_FOUND:
                case REPLICANT_COND_NOT_FOUND:
                case REPLICANT_BAD_LIBRARY:
                case REPLICANT_TIMEOUT:
                case REPLICANT_INTERNAL_ERROR:
                case REPLICANT_NONE_PENDING:
                case REPLICANT_GARBAGE:
                default:
                    *status = HYPERCLIENT_INTERNAL;
                    return false;
            }

            abort();
        }
    }

    assert(m_get_config_id >= 0);
    replicant_returncode lstatus;
    int64_t lid = m_repl.loop(0, &lstatus);

    if (lid < 0)
    {
        switch (lstatus)
        {
            case REPLICANT_SERVER_ERROR:
            case REPLICANT_NEED_BOOTSTRAP:
            case REPLICANT_MISBEHAVING_SERVER:
                *status = HYPERCLIENT_COORDFAIL;
                return false;
            case REPLICANT_TIMEOUT:
                return true;
            case REPLICANT_SUCCESS:
            case REPLICANT_NAME_TOO_LONG:
            case REPLICANT_FUNC_NOT_FOUND:
            case REPLICANT_OBJ_EXIST:
            case REPLICANT_OBJ_NOT_FOUND:
            case REPLICANT_COND_NOT_FOUND:
            case REPLICANT_BAD_LIBRARY:
            case REPLICANT_INTERNAL_ERROR:
            case REPLICANT_NONE_PENDING:
            case REPLICANT_GARBAGE:
            default:
                *status = HYPERCLIENT_INTERNAL;
                return false;
        }

        abort();
    }

    if (lid != m_get_config_id)
    {
        if (m_get_config_output)
        {
            replicant_destroy_output(m_get_config_output, m_get_config_output_sz);
        }

        m_repl.kill(m_get_config_id);
        m_get_config_id = -1;
        *status = HYPERCLIENT_INTERNAL;
        return false;
    }

    switch (m_get_config_status)
    {
        case REPLICANT_SUCCESS:
            break;
        case REPLICANT_FUNC_NOT_FOUND:
        case REPLICANT_OBJ_NOT_FOUND:
        case REPLICANT_COND_NOT_FOUND:
        case REPLICANT_SERVER_ERROR:
        case REPLICANT_NEED_BOOTSTRAP:
        case REPLICANT_MISBEHAVING_SERVER:
            *status = HYPERCLIENT_COORDFAIL;
            return false;
        case REPLICANT_NAME_TOO_LONG:
        case REPLICANT_OBJ_EXIST:
        case REPLICANT_BAD_LIBRARY:
        case REPLICANT_TIMEOUT:
        case REPLICANT_INTERNAL_ERROR:
        case REPLICANT_NONE_PENDING:
        case REPLICANT_GARBAGE:
        default:
            *status = HYPERCLIENT_INTERNAL;
            return false;
    }

std::cout << "config is of size " << m_get_config_output_sz << "b" << std::endl;
    e::unpacker up(m_get_config_output, m_get_config_output_sz);
    configuration new_config;
    up = up >> new_config;
    replicant_destroy_output(m_get_config_output, m_get_config_output_sz);

    if (up.error())
    {
        *status = HYPERCLIENT_BADCONFIG;
        return false;
    }

    m_config = new_config;
    return true;
}

int
coordinator_link :: poll_fd()
{
    return m_repl.poll_fd();
}

bool
coordinator_link :: make_rpc(const char* func,
                             const char* data, size_t data_sz,
                             hyperclient_returncode* status,
                             const char** output, size_t* output_sz)
{
    replicant_returncode sstatus;
    int64_t sid = m_repl.send("hyperdex", func, data, data_sz,
                              &sstatus, output, output_sz);

    if (sid < 0)
    {
        switch (sstatus)
        {
            case REPLICANT_SERVER_ERROR:
            case REPLICANT_NEED_BOOTSTRAP:
            case REPLICANT_MISBEHAVING_SERVER:
                *status = HYPERCLIENT_COORDFAIL;
                return false;
            case REPLICANT_SUCCESS:
            case REPLICANT_NAME_TOO_LONG:
            case REPLICANT_FUNC_NOT_FOUND:
            case REPLICANT_OBJ_EXIST:
            case REPLICANT_OBJ_NOT_FOUND:
            case REPLICANT_COND_NOT_FOUND:
            case REPLICANT_BAD_LIBRARY:
            case REPLICANT_TIMEOUT:
            case REPLICANT_INTERNAL_ERROR:
            case REPLICANT_NONE_PENDING:
            case REPLICANT_GARBAGE:
            default:
                *status = HYPERCLIENT_INTERNAL;
                return false;
        }
    }

    replicant_returncode lstatus;
    int64_t lid = m_repl.loop(sid, -1, &lstatus);

    if (lid < 0)
    {
        switch (lstatus)
        {
            case REPLICANT_SERVER_ERROR:
            case REPLICANT_NEED_BOOTSTRAP:
            case REPLICANT_MISBEHAVING_SERVER:
                *status = HYPERCLIENT_COORDFAIL;
                return false;
            case REPLICANT_TIMEOUT:
            case REPLICANT_SUCCESS:
            case REPLICANT_NAME_TOO_LONG:
            case REPLICANT_FUNC_NOT_FOUND:
            case REPLICANT_OBJ_EXIST:
            case REPLICANT_OBJ_NOT_FOUND:
            case REPLICANT_COND_NOT_FOUND:
            case REPLICANT_BAD_LIBRARY:
            case REPLICANT_INTERNAL_ERROR:
            case REPLICANT_NONE_PENDING:
            case REPLICANT_GARBAGE:
            default:
                *status = HYPERCLIENT_INTERNAL;
                return false;
        }
    }

    switch (sstatus)
    {
        case REPLICANT_SUCCESS:
            return true;
        case REPLICANT_FUNC_NOT_FOUND:
        case REPLICANT_OBJ_NOT_FOUND:
        case REPLICANT_COND_NOT_FOUND:
        case REPLICANT_SERVER_ERROR:
        case REPLICANT_NEED_BOOTSTRAP:
        case REPLICANT_MISBEHAVING_SERVER:
            *status = HYPERCLIENT_COORDFAIL;
            return false;
        case REPLICANT_NAME_TOO_LONG:
        case REPLICANT_OBJ_EXIST:
        case REPLICANT_BAD_LIBRARY:
        case REPLICANT_TIMEOUT:
        case REPLICANT_INTERNAL_ERROR:
        case REPLICANT_NONE_PENDING:
        case REPLICANT_GARBAGE:
        default:
            *status = HYPERCLIENT_INTERNAL;
            return false;
    }
}
