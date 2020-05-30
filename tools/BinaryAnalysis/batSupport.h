#ifndef ROSE_BatSupport_H
#define ROSE_BatSupport_H

#include <rose.h>
#include <BinaryFeasiblePath.h>                         // rose
#include <BinarySerialIo.h>                             // rose
#include <Partitioner2/Function.h>                      // rose
#include <Partitioner2/Partitioner.h>                   // rose

#include <fstream>
#include <Sawyer/CommandLine.h>
#include <Sawyer/Message.h>
#include <set>
#include <string>

// Minimum ROSE versions required by this tool
#define MINIMUM_ROSE_HEADER_VERSION 9013017ul
#define MINIMUM_ROSE_LIBRARY_VERSION "0.9.13.17"

#if !defined(ROSE_VERSION)
    #warning "unknown ROSE version"
#elif ROSE_VERSION < MINIMUM_ROSE_HEADER_VERSION
    #error "ROSE headers version is too old"
#endif

/** Support for ROSE binary analysis tools. */
namespace Bat {

/** Whether to print state information. */
namespace ShowStates {
    /** Whether to print state information. */
    enum Flag {
        NO,                                             /**< Do not print state. */
        YES                                             /**< Print state. */
    };
}

/** Checks whether the ROSE version number is sufficient.
 *
 *  Given a version number string, like "0.9.7.128", return true if the ROSE library is that version or later, and false
 *  otherwise.  The versions that takes a stream argument will print an error message and exit.
 *
 * @{ */
bool checkRoseVersionNumber(const std::string &minimumVersion);
void checkRoseVersionNumber(const std::string &minimumVersion, Sawyer::Message::Stream &fatalStream);
/** @} */

/** Create a command-line switch for the state file format. */
Sawyer::CommandLine::Switch stateFileFormatSwitch(Rose::BinaryAnalysis::SerialIo::Format &fmt/*in,out*/);

/** Check that output file name is valid or exit. */
void checkRbaOutput(const boost::filesystem::path &name, Sawyer::Message::Facility &mlog);

/** Select functions by name or address.
 *
 *  Given a list of functions, select those that have a name or address that matches any in @p namesOrAddresses.  Names are
 *  matched exactly, and addresses are matched numerically. The returned vector's functions will be in the same order as the
 *  input vector.
 *
 *  Any elements of @p namesOrAddresses that didn't match any of the @p functions is added to the @p unmatched set, or if an
 *  error stream is provided instead will be reported to that stream.
 *
 * @{ */
std::vector<Rose::BinaryAnalysis::Partitioner2::Function::Ptr>
selectFunctionsByNameOrAddress(const std::vector<Rose::BinaryAnalysis::Partitioner2::Function::Ptr> &functions,
                               const std::set<std::string> &namesOrAddresses, std::set<std::string> &unmatched /*in,out*/);
std::vector<Rose::BinaryAnalysis::Partitioner2::Function::Ptr>
selectFunctionsByNameOrAddress(const std::vector<Rose::BinaryAnalysis::Partitioner2::Function::Ptr> &functions,
                               const std::set<std::string> &namesOrAddresses, Sawyer::Message::Stream&);
/** @} */

/** Select functions that contain an address.
 *
 *  Returns a list of functions sorted by primary entry address such that each function spans the specified address. In other
 *  words, the address can be the function entry address, a basic block address, an instruction address, or any address within
 *  an instruction. */
std::vector<Rose::BinaryAnalysis::Partitioner2::Function::Ptr>
selectFunctionsContainingInstruction(const Rose::BinaryAnalysis::Partitioner2::Partitioner &partitioner,
                                     const std::set<rose_addr_t> &insnVas);

/** Select CFG vertex by name or address. */
Rose::BinaryAnalysis::Partitioner2::ControlFlowGraph::ConstVertexIterator
vertexForInstruction(const Rose::BinaryAnalysis::Partitioner2::Partitioner &partitioner, const std::string &nameOrVa);

/** Select CFG edge by endpoint vertices. */
Rose::BinaryAnalysis::Partitioner2::ControlFlowGraph::ConstEdgeIterator
edgeForInstructions(const Rose::BinaryAnalysis::Partitioner2::Partitioner &partitioner,
                    const Rose::BinaryAnalysis::Partitioner2::ControlFlowGraph::ConstVertexIterator &source,
                    const Rose::BinaryAnalysis::Partitioner2::ControlFlowGraph::ConstVertexIterator &target);

/** Select CFG edge by endpoint names. */
Rose::BinaryAnalysis::Partitioner2::ControlFlowGraph::ConstEdgeIterator
edgeForInstructions(const Rose::BinaryAnalysis::Partitioner2::Partitioner &partitioner, const std::string &sourceNameOrVa,
                    const std::string &targetNameOrVa);

/** Register all the Bat self tests for the --self-test switch. */
void registerSelfTests();

/** Return the first and last function names from the path if known. */
std::pair<std::string, std::string>
pathEndpointFunctionNames(const Rose::BinaryAnalysis::FeasiblePath&, const Rose::BinaryAnalysis::Partitioner2::CfgPath&);

/** Print information about a path from the model checker. */
void
printPath(std::ostream&, const Rose::BinaryAnalysis::FeasiblePath&,
          const Rose::BinaryAnalysis::Partitioner2::CfgPath&, const Rose::BinaryAnalysis::SmtSolverPtr&,
          const Rose::BinaryAnalysis::InstructionSemantics2::BaseSemantics::RiscOperatorsPtr&,
          SgAsmInstruction *lastInsn, ShowStates::Flag);

/** Compute calling conventions. */
void assignCallingConventions(const Rose::BinaryAnalysis::Partitioner2::Partitioner&);

/** Selects whether a path is to be printed. */
class PathSelector {
public:
    // Settings
    bool suppressUninteresting;                         /**< Suppress paths that are "uninteresting". */
    bool suppressDuplicatePaths;                        /**< Suppress paths that have the same CFG execution sequence. */
    bool suppressDuplicateEndpoints;                    /**< Suppress paths that end at the same address. */
    size_t maxPaths;                                    /**< Maximum number of paths to show. */
    std::set<uint64_t> requiredHashes;                  /**< If non-empty, show only paths matching these hashes. */

private:
    SAWYER_THREAD_TRAITS::Mutex mutex_;                 // protects the following data members
    std::set<uint64_t> seenPaths_;
    std::set<rose_addr_t> seenEndpoints_;
    size_t nUninteresting_;                             // number of uninteresting paths suppressed
    size_t nDuplicatePaths_;                            // number of duplicate paths suppressed
    size_t nDuplicateEndpoints_;                        // number of duplicate endpoints suppressed
    size_t nWrongHashes_;                               // number of suppressions due to not being a specified hash
    size_t nSelected_;                                  // number of times operator() returned true

public:
    PathSelector()
        : suppressUninteresting(false), suppressDuplicatePaths(false), suppressDuplicateEndpoints(false),
          maxPaths(Rose::UNLIMITED), nUninteresting_(0), nDuplicatePaths_(0), nDuplicateEndpoints_(0), nWrongHashes_(0),
          nSelected_(0) {}

public:
    /** Reset statistics. */
    void resetStats() {
        nUninteresting_ = nDuplicatePaths_ = nDuplicateEndpoints_ = nSelected_ = 0;
    }

    /** Return non-zero if the path should be shown, zero if suppressed.
     *
     *  The specified path is compared against previously checked paths to see whether it should be presented to the user
     *  based on settings and indexes contained in this object. Checking whether a path should be shown also updates various
     *  indexes.
     *
     *  If the path is suitable for being shown to the user, then this function returns a non-zero path number that's
     *  incremented each time this method is called.
     *
     *  Thread safety: This method is thread safe if the user is not concurrently adjusting this object's settings. */
    size_t operator()(const Rose::BinaryAnalysis::FeasiblePath &fpAnalysis,
                      const Rose::BinaryAnalysis::Partitioner2::CfgPath &path,
                      SgAsmInstruction *offendingInstruction);

    /** Property: Number of uninteresting paths suppressed. */
    size_t nUninteresting() const { return nUninteresting_; }

    /** Property: Number of paths suppressed due to non-matching hashes. */
    size_t nWrongHashes() const { return nWrongHashes_; }

    /** Property: Number of duplicate paths suppressed.
     *
     *  This does not include paths that were suppressed because they were deemed to be uninteresting. */
    size_t nDuplicatePaths() const { return nDuplicatePaths_; }

    /** Property: Number of duplicate endpoints suppressed.
     *
     *  This does not include paths that were suppressed because they were deemed to be unintersting or because the entire path
     *  was a duplicate. */
    size_t nDuplicateEndpoints() const { return nDuplicateEndpoints_; }

    /** Property: Number of paths selected.
     *
     *  The number of times that the function operator returned a non-zero value. */
    size_t nSelected() const { return nSelected_; }

    /** Terminate entire program if we've selected the maximum number of paths.
     *
     *  It's not safe to call "exit" in a multi-threaded application when other threads are still executing (expect failed
     *  assertions in boost and other weirdness), so we use "_exit" instead, which is an immediate termination of the process
     *  without any in-process cleanup. */
    void maybeTerminate() const;
};

} // namespace

#endif
