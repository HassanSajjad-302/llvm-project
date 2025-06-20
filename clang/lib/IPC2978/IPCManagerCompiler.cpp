
#include "clang/IPC2978/IPCManagerCompiler.hpp"
#include "clang/IPC2978/Manager.hpp"
#include "clang/IPC2978/Messages.hpp"

#include <string>
#include <Windows.h>

using std::string;

namespace N2978
{

tl::expected<void, string> IPCManagerCompiler::receiveBTCLastMessage() const
{
    char buffer[BUFFERSIZE];
    uint32_t bytesRead;
    if (const auto &r = read(buffer); !r)
    {
        return tl::unexpected(r.error());
    }
    else
    {
        bytesRead = *r;
    }

    if (buffer[0] != false)
    {
        IPCErr(ErrorCategory::INCORRECT_BTC_LAST_MESSAGE)
    }

    if (constexpr uint32_t bytesProcessed = 1; bytesRead != bytesProcessed)
    {
        return tl::unexpected(string{});
    }

    return {};
}

IPCManagerCompiler::IPCManagerCompiler(const string &objOrBMIFilePath) : pipeName(R"(\\.\pipe\)" + objOrBMIFilePath)
{
}

tl::expected<void, string> IPCManagerCompiler::connectToBuildSystem()
{
    if (connectedToBuildSystem)
    {
        return {};
    }

    hPipe = CreateFileA(pipeName.data(), // pipe name
                       GENERIC_READ |   // read and write access
                           GENERIC_WRITE,
                       0,             // no sharing
                       nullptr,       // default security attributes
                       OPEN_EXISTING, // opens existing pipe
                       0,             // default attributes
                       nullptr);      // no template file

    // Break if the pipe handle is valid.

    if (hPipe == INVALID_HANDLE_VALUE)
    {
        return tl::unexpected(string{});
    }

    connectedToBuildSystem = true;
    return {};
}

tl::expected<BTCModule, string> IPCManagerCompiler::receiveBTCModule(const CTBModule &moduleName)
{
    if (const auto &r = connectToBuildSystem(); !r)
    {
        IPCErr(r.error())
    }
    vector<char> buffer = getBufferWithType(CTB::MODULE);
    writeString(buffer, moduleName.moduleName);
    if (const auto &r = write(buffer); !r)
    {
        IPCErr(r.error());
    }

    return receiveMessage<BTCModule>();
}

tl::expected<BTCNonModule, string> IPCManagerCompiler::receiveBTCNonModule(const CTBNonModule &nonModule)
{
    if (const auto &r = connectToBuildSystem(); !r)
    {
        IPCErr(r.error())
    }
    vector<char> buffer = getBufferWithType(CTB::NON_MODULE);
    buffer.emplace_back(nonModule.isHeaderUnit);
    writeString(buffer, nonModule.str);
    if (const auto &r = write(buffer); !r)
    {
        IPCErr(r.error())
    }
    return receiveMessage<BTCNonModule>();
}

tl::expected<void, string> IPCManagerCompiler::sendCTBLastMessage(const CTBLastMessage &lastMessage)
{
    if (const auto &r = connectToBuildSystem(); !r)
    {
        IPCErr(r.error())
    }
    vector<char> buffer = getBufferWithType(CTB::LAST_MESSAGE);
    buffer.emplace_back(lastMessage.exitStatus);
    writeVectorOfStrings(buffer, lastMessage.headerFiles);
    writeString(buffer, lastMessage.output);
    writeString(buffer, lastMessage.errorOutput);
    writeString(buffer, lastMessage.logicalName);
    if (const auto &r = write(buffer); !r)
    {
        IPCErr(r.error())
    }
    return {};
}

tl::expected<void, string> IPCManagerCompiler::sendCTBLastMessage(const CTBLastMessage &lastMessage,
                                                                  const string &bmiFile, const string &filePath)
{
    const HANDLE hFile = CreateFileA(filePath.c_str(), GENERIC_READ | GENERIC_WRITE,
                                    0, // no sharing during setup
                                    nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return tl::unexpected(string{});
    }

    LARGE_INTEGER fileSize;
    fileSize.QuadPart = bmiFile.size();
    // 3) Create a RW mapping of that file:
    const HANDLE hMap =
        CreateFileMappingA(hFile, nullptr, PAGE_READWRITE, fileSize.HighPart, fileSize.LowPart, filePath.c_str());
    if (!hMap)
    {
        CloseHandle(hFile);
        return tl::unexpected(string{});
    }

    void *pView = MapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0, bmiFile.size());
    if (!pView)
    {
        CloseHandle(hFile);
        CloseHandle(hMap);
        return tl::unexpected(string{});
    }

    memcpy(pView, bmiFile.c_str(), bmiFile.size());

    if (!FlushViewOfFile(pView, bmiFile.size()))
    {
        UnmapViewOfFile(pView);
        CloseHandle(hFile);
        CloseHandle(hMap);
        return tl::unexpected(string{});
    }

    UnmapViewOfFile(pView);
    CloseHandle(hFile);

    if (const auto &r = sendCTBLastMessage(lastMessage); !r)
    {
        IPCErr(r.error())
    }

    if (lastMessage.exitStatus == EXIT_SUCCESS)
    {
        if (const auto &r = receiveBTCLastMessage(); !r)
        {
            IPCErr(r.error())
        }
    }

    CloseHandle(hMap);
    return {};
}

tl::expected<string_view, string> IPCManagerCompiler::readSharedMemoryBMIFile(const BMIFile &file)
{
    // 1) Open the existing file‐mapping object (must have been created by another process)
    const HANDLE mapping = OpenFileMappingA(FILE_MAP_READ,       // read‐only access
                                           FALSE,               // do not inherit a handle
                                           file.filePath.data() // name of mapping
    );

    if (mapping == nullptr)
    {
        return tl::unexpected(string{});
    }

    // 2) Map a view of the file into our address space
    const LPVOID view = MapViewOfFile(mapping,       // handle to mapping object
                                      FILE_MAP_READ, // read‐only view
                                      0,             // file offset high
                                      0,             // file offset low
                                      file.fileSize  // number of bytes to map (0 maps the whole file)
    );

    if (view == nullptr)
    {
        CloseHandle(mapping);
        return tl::unexpected(string{});
    }

    MemoryMappedBMIFile f;
    f.mapping = mapping;
    f.view = view;
    memoryMappedBMIFiles.emplace_back(std::move(f));
    return string_view{static_cast<char *>(view), file.fileSize};
}
} // namespace N2978