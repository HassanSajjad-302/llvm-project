
#ifndef IPC_MANAGER_BS_HPP
#define IPC_MANAGER_BS_HPP

#include "clang/IPC2978/Manager.hpp"
#include "clang/IPC2978/Messages.hpp"

namespace N2978
{

// IPC Manager BuildSystem
class IPCManagerBS : public Manager
{
    friend tl::expected<IPCManagerBS, string> makeIPCManagerBS(string objOrCompilerFilePath);
    bool connectedToCompiler = false;

    tl::expected<void, string> connectToCompiler() const;

    explicit IPCManagerBS(void *hPipe_);

  public:
    IPCManagerBS(const IPCManagerBS &) = default;
    IPCManagerBS &operator=(const IPCManagerBS &) = default;
    IPCManagerBS(IPCManagerBS &&) = default;
    IPCManagerBS &operator=(IPCManagerBS &&) = default;
    tl::expected<void, string> receiveMessage(char (&ctbBuffer)[320], CTB &messageType) const;
    tl::expected<void, string> sendMessage(const BTCModule &moduleFile) const;
    tl::expected<void, string> sendMessage(const BTCNonModule &nonModule) const;
    tl::expected<void, string> sendMessage(const BTCLastMessage &lastMessage) const;
    static tl::expected<void *, string> createSharedMemoryBMIFile(const string &bmiFilePath);
};

tl::expected<IPCManagerBS, string> makeIPCManagerBS(string objOrCompilerFilePath);
} // namespace N2978
#endif // IPC_MANAGER_BS_HPP
