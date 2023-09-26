/*
 * Copyright (c) 2023 Clement Vuchener
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */
#include "PluginManager.h"
#include "RemoteServer.h"
#include "MemAccess.h"

#include "llmemreader.pb.h"

#if defined(WIN32)
extern "C" {
#include <windows.h>
}
#elif defined(_LINUX)
extern "C" {
#include <unistd.h>
#include <sys/uio.h>
}
#endif

using namespace DFHack;
using namespace dfproto::llmemoryreader;

DFHACK_PLUGIN("llmemreader");

DFhackCExport command_result plugin_init(color_ostream &out, std::vector<PluginCommand> &)
{
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out)
{
    return CR_OK;
}

static command_result get_info(color_ostream &stream, const EmptyMessage *, Info *out)
{
    const auto &process = Core::getInstance().p;
    auto version_info = process->getDescriptor();
    out->set_version(version_info->getVersion());
#if defined(WIN32)
    out->set_pe(process->getPE());
#else
    out->set_md5(process->getMD5());
#endif
    out->set_base_offset(version_info->getRebaseDelta());
    return CR_OK;
}

static command_result read_raw(color_ostream &stream, const ReadRawIn *in, ReadRawOut *out)
{
    void *address = reinterpret_cast<void *>(in->address());
    std::size_t length = in->length();
    std::unique_ptr<char[]> data(new char[length]);
#if defined(WIN32)
    auto current_process = GetCurrentProcess();
    SIZE_T bytes_read;
    if (!ReadProcessMemory(current_process, address, data.get(), length, &bytes_read)) {
        auto err = GetLastError();
        LPSTR msg;
        auto msg_len = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&msg, 0, NULL);
        out->set_error_message(msg, msg_len);
        LocalFree(msg);
    }
    else {
        out->set_data(data.get(), bytes_read);
    }
    return CR_OK;
#elif defined(_LINUX)
    auto current_process = getpid();
    iovec local = {data.get(), length};
    iovec remote = {address, length};
    auto bytes_read = process_vm_readv(current_process, &local, 1, &remote, 1, 0);
    if (bytes_read == -1) {
        auto err = errno;
        out->set_error_message(strerror(err));
    }
    else {
        out->set_data(data.get(), bytes_read);
    }
    return CR_OK;
//#elif defined(_DARWIN)
//  TODO
#else
    return CR_NOT_IMPLEMENTED;
#endif
}

static command_result read_raw_v(color_ostream &stream, const ReadRawVIn *in, ReadRawVOut *out)
{
    out->mutable_list()->Reserve(in->list_size());
    for (const auto &read_raw_in: in->list()) {
        auto cr = read_raw(stream, &read_raw_in, out->add_list());
        if (cr != CR_OK)
            return cr;
    }
    return CR_OK;
}

DFhackCExport RPCService *plugin_rpcconnect(color_ostream &)
{
    RPCService *svc = new RPCService();
    svc->addFunction("GetInfo", get_info, SF_ALLOW_REMOTE);
    svc->addFunction("ReadRaw", read_raw, SF_ALLOW_REMOTE);
    svc->addFunction("ReadRawV", read_raw_v, SF_ALLOW_REMOTE);
    return svc;
}
