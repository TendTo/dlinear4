#include "info_gatherer.hh"

namespace dreal {

    InfoGatherer::InfoGatherer(const string &filename, const string &solver,
                               const string &precision)
            : config_{}, filename_{filename}, solver_{solver}, timeout_{0} {
        precision_ = stod(precision);
        config_.mutable_lp_solver().set_from_command_line(GetLPSolver(solver_));
        config_.mutable_precision().set_from_command_line(precision_);
    }

    InfoGatherer::InfoGatherer(const string &filename, const string &solver,
                               const string &precision, int timeout)
            : config_{}, filename_{filename}, solver_{solver}, timeout_{timeout} {
        precision_ = stod(precision);
        config_.mutable_lp_solver().set_from_command_line(GetLPSolver(solver_));
        config_.mutable_precision().set_from_command_line(precision_);
    }

    void InfoGatherer::SetDebugScanning(bool debugScanning) {
        debug_scanning_ = debugScanning;
    }

    void InfoGatherer::SetDebugParsing(bool debugParsing) {
        debug_parsing_ = debugParsing;
    }

    bool InfoGatherer::Run() {
        std::cout << "Running " << filename_ << " with " << solver_ << " and "
                  << precision_ << std::endl;
        // Shared memory to store the results of the child process.
        auto *results = static_cast<shared_results *>(mmap(nullptr, sizeof(shared_results),
                                                           PROT_READ | PROT_WRITE,
                                                           MAP_SHARED | MAP_ANONYMOUS, -1,
                                                           0));
        // Fork an intermediate process to fork a worker process and a timer process.
        StartIntermediateProcess(results);
        // Wait for the child process to finish and get the exit status
        bool res = WaitChild();
        if (res) ParseResults(results);
        // Unmap the shared memory.
        munmap(results, sizeof(shared_results));
        // Return true if the child process exits with 0, meaning no error has
        // occurred
        return res;
    }

    void InfoGatherer::StartIntermediateProcess(shared_results *results) {
        intermediate_pid_ = fork();
        if (intermediate_pid_ == 0) {
            pid_t worker_pid = -1, timer_pid = -1;
            worker_pid = fork();
            if (worker_pid == 0) {
                Init();
                GatherInfo(results);
                DeInit();
                exit(0);
            }
            if (worker_pid == -1) {
                throw DREAL_RUNTIME_ERROR("Failed to fork a worker process.");
            }

            if (timeout_ > 0) {
                timer_pid = fork();
                if (timer_pid == 0) {
                    sleep(timeout_);
                    exit(0);
                }
                if (timer_pid == -1) {
                    throw DREAL_RUNTIME_ERROR("Failed to fork a timer process.");
                }
            }

            int status;
            pid_t pid = wait(&status);
            if (pid == worker_pid) {
                if (timer_pid > 0) kill(timer_pid, SIGKILL);
            } else if (pid == timer_pid) {
                if (worker_pid > 0) kill(worker_pid, SIGKILL);
            } else
                throw DREAL_RUNTIME_ERROR("Unexpected child process {} exited with {}",
                                          pid, status);
            exit(pid == worker_pid ? status : 1);
        }
        if (intermediate_pid_ == -1) {
            throw DREAL_RUNTIME_ERROR("Failed to fork an intermediate process.");
        }
    }

    bool InfoGatherer::WaitChild() {
        int status;
        int pid = waitpid(intermediate_pid_, &status, 0);
        if (pid == -1) {
            throw DREAL_RUNTIME_ERROR("Failed to wait for the worker process.");
        }
        return status == 0;
    }

    void InfoGatherer::ParseResults(shared_results *results) {
        nAssertions_ = results->_nAssertions;
        isSat_ = results->_isSat;
        actualPrecision_ = results->_actualPrecision;
    }

    const string &InfoGatherer::filename() const { return filename_; }

    const string &InfoGatherer::solver() const { return solver_; }

    double InfoGatherer::precision() const { return precision_; }

    double InfoGatherer::actualPrecision() const { return actualPrecision_; }

    size_t InfoGatherer::nAssertions() const { return nAssertions_; }

    bool InfoGatherer::isSat() const { return isSat_; }

    string InfoGatherer::GetSolverName(const Config::LPSolver solver) {
        switch (solver) {
            case Config::SOPLEX:
                return "soplex";
            case Config::QSOPTEX:
                return "qsoptex";
            default:
                throw DREAL_RUNTIME_ERROR("Unknown solver {}", solver);
        }
    }

    Config::LPSolver InfoGatherer::GetLPSolver(const string &solver) {
        if (solver == "soplex") {
            return Config::SOPLEX;
        } else if (solver == "qsoptex") {
            return Config::QSOPTEX;
        }
        throw DREAL_RUNTIME_ERROR("Unknown solver {}", solver);
    }

    void InfoGatherer::Init() {
        if (config_.lp_solver() == Config::QSOPTEX) {
            qsopt_ex::QSXStart();
            InftyStart(qsopt_ex::mpq_INFTY, qsopt_ex::mpq_NINFTY);
        } else if (config_.lp_solver() == Config::SOPLEX) {
            InftyStart(soplex::infinity);
        }
        Expression::InitConstants();
    }

    void InfoGatherer::DeInit() {
        Expression::DeInitConstants();
        InftyFinish();
        if (config_.lp_solver() == Config::QSOPTEX) {
            qsopt_ex::QSXFinish();
        }
    }

    void InfoGatherer::GatherInfo(shared_results *results) {
        Smt2Driver smt2_driver{Context{Config{config_}}};
        smt2_driver.set_trace_scanning(debug_scanning_);
        smt2_driver.set_trace_parsing(debug_parsing_);
        smt2_driver.parse_file(filename_);
        results->_nAssertions = smt2_driver.context().assertions().size();
        results->_isSat = !smt2_driver.context().get_model().empty();
        results->_actualPrecision = smt2_driver.actual_precision();
    }

}  // namespace dreal
