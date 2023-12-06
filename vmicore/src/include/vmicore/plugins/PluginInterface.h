#ifndef VMICORE_PLUGININTERFACE_H
#define VMICORE_PLUGININTERFACE_H

#include "../io/ILogger.h"
#include "../os/ActiveProcessInformation.h"
#include "../types.h"
#include "../vmi/BpResponse.h"
#include "../vmi/IBreakpoint.h"
#include "../vmi/IIntrospectionAPI.h"
#include "../vmi/events/IInterruptEvent.h"
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace VmiCore::Plugin
{
    /**
     * Contains all functionality that is exposed to plugins.
     */
    class PluginInterface
    {
      public:
        constexpr static uint8_t API_VERSION = 15;

        virtual ~PluginInterface() = default;

        /**
         * Reads a region of contiguous virtual memory from a process. The starting offset as well as the size must be
         * 4kb page aligned.
         *
         * @return A unique pointer to a byte vector containing the memory content. Subregions that could not be
         * extracted (e.g. because they are paged out) will be replaced by a single all zero padding page.
         */
        [[nodiscard]] virtual std::unique_ptr<std::vector<uint8_t>>
        readProcessMemoryRegion(pid_t pid, addr_t address, size_t numberOfBytes) const = 0;

        /**
         * Obtain a vector containing an OS-agnostic representation of all currently running processes.
         * The vector is a snapshot of the current state, it won't receive any updates.
         */
        [[nodiscard]] virtual std::unique_ptr<std::vector<std::shared_ptr<const ActiveProcessInformation>>>
        getRunningProcesses() const = 0;

        /**
         * Subscribe to process start events. The supplied lambda function will be called once the event occurs.
         *
         * @param startCallback It is recommended to create the lambda with the help of VMICORE_SETUP_MEMBER_CALLBACK
         * from <a href="file:../callback.h">callback.h</a>
         */
        virtual void registerProcessStartEvent(
            const std::function<void(std::shared_ptr<const ActiveProcessInformation>)>& startCallback) = 0;

        /**
         * Subscribe to process termination events. The supplied lambda function will be called once the event occurs.
         *
         * @param terminationCallback It is recommended to create the lambda with the help of
         * VMICORE_SETUP_MEMBER_CALLBACK from <a href="file:../callback.h">callback.h</a>
         */
        virtual void registerProcessTerminationEvent(
            const std::function<void(std::shared_ptr<const ActiveProcessInformation>)>& terminationCallback) = 0;

        /**
         * Create a software breakpoint at the given virtual address. The breakpoint will be protected, so that
         * it won't be visible to the guest through reading the memory. Multiple breakpoints per address are allowed and
         * will not interfere with each other. If the breakpoint is hit, the supplied callback will be called.
         *
         * @param targetVA Target address to place the breakpoint on.
         * @param processInformation The process information for the target process. Can be obtained via
         * getRunningProcesses().
         * @param callbackFunction It is recommended to create the lambda with the help of VMICORE_SETUP_MEMBER_CALLBACK
         * from <a href="file:../callback.h">callback.h</a>
         * @return Shared pointer to a breakpoint object. Can be used to delete the breakpoint.
         */
        [[nodiscard]] virtual std::shared_ptr<IBreakpoint>
        createBreakpoint(uint64_t targetVA,
                         const ActiveProcessInformation& processInformation,
                         const std::function<BpResponse(IInterruptEvent&)>& callbackFunction) = 0;

        /**
         * Retrieves the path to the directory where plugins are supposed to store any files that are generated
         * throughout the course of a run. However, it is generally discouraged to store files directly. Instead,
         * the writeToFile or the logging APIs should be used.
         */
        [[nodiscard]] virtual std::unique_ptr<std::string> getResultsDir() const = 0;

        /**
         * Creates a new logger with a given name.
         */
        [[nodiscard]] virtual std::unique_ptr<ILogger> newNamedLogger(std::string_view name) const = 0;

        /**
         * Saves content to a file with the given name. Does not append. Saving the same file more than once is
         * undefined behavior. Use the other overload for raw data.
         */
        virtual void writeToFile(const std::string& filename, const std::string& message) const = 0;

        /**
         * Saves content to a file with the given name. Does not append. Saving the same file more than once is
         * undefined behavior. Use the other overload for strings.
         */
        virtual void writeToFile(const std::string& filename, const std::vector<uint8_t>& data) const = 0;

        /**
         * Only useful if using a gRPC connection, does nothing otherwise. Will send an error event via a separate
         * channel which indicates that the run is not successful.
         */
        virtual void sendErrorEvent(std::string_view message) const = 0;

        /**
         * Only useful if using a gRPC connection, does nothing otherwise. Will send an inmemory scanner detection event
         * via a gRPC channel.
         */
        virtual void sendInMemDetectionEvent(std::string_view message) const = 0;

        /**
         * Gives access to low level introspection API. Interface is limited to a subset of calls that are deemed
         * non-invasive in order to avoid interfering with other plugins. Note that any call made through the
         * introspection API object will acquire an API-wide lock because the underlying implementation is not
         * considered thread safe.
         */
        [[nodiscard]] virtual std::shared_ptr<IIntrospectionAPI> getIntrospectionAPI() const = 0;

      protected:
        PluginInterface() = default;
    };
}

#endif // VMICORE_PLUGININTERFACE_H
