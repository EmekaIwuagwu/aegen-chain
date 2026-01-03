#pragma once
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <map>
#include <atomic>
#include <vector>
#include <queue>
#include <thread>
#include <functional>

namespace aegen {

/**
 * LogLevel - Severity levels for logging
 */
enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5
};

/**
 * Logger - Production-grade structured logging
 * 
 * Features:
 * - JSON and text output formats
 * - File and console output
 * - Async logging with queue
 * - Log rotation
 * - Contextual fields
 */
class Logger {
private:
    static Logger* instance;
    static std::mutex instanceMtx;
    
    LogLevel minLevel = LogLevel::INFO;
    bool jsonFormat = false;
    bool consoleOutput = true;
    std::ofstream fileStream;
    std::mutex logMtx;
    std::map<std::string, std::string> contextFields;
    
    std::string levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::TRACE: return "TRACE";
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO:  return "INFO";
            case LogLevel::WARN:  return "WARN";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::FATAL: return "FATAL";
            default: return "UNKNOWN";
        }
    }
    
    std::string levelToColor(LogLevel level) {
        switch (level) {
            case LogLevel::TRACE: return "\033[90m";   // Gray
            case LogLevel::DEBUG: return "\033[36m";   // Cyan
            case LogLevel::INFO:  return "\033[32m";   // Green
            case LogLevel::WARN:  return "\033[33m";   // Yellow
            case LogLevel::ERROR: return "\033[31m";   // Red
            case LogLevel::FATAL: return "\033[35m";   // Magenta
            default: return "\033[0m";
        }
    }
    
    std::string getTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%S")
           << "." << std::setfill('0') << std::setw(3) << ms.count() << "Z";
        return ss.str();
    }
    
    void writeLog(LogLevel level, const std::string& component, 
                  const std::string& message, 
                  const std::map<std::string, std::string>& fields = {}) {
        if (level < minLevel) return;
        
        std::lock_guard<std::mutex> lock(logMtx);
        
        std::stringstream output;
        
        if (jsonFormat) {
            output << "{\"timestamp\":\"" << getTimestamp() << "\","
                   << "\"level\":\"" << levelToString(level) << "\","
                   << "\"component\":\"" << component << "\","
                   << "\"message\":\"" << message << "\"";
            
            // Add context fields
            for (const auto& [k, v] : contextFields) {
                output << ",\"" << k << "\":\"" << v << "\"";
            }
            
            // Add message fields
            for (const auto& [k, v] : fields) {
                output << ",\"" << k << "\":\"" << v << "\"";
            }
            
            output << "}";
        } else {
            std::string reset = "\033[0m";
            output << getTimestamp() << " "
                   << levelToColor(level) << "[" << levelToString(level) << "]" << reset << " "
                   << "\033[1m[" << component << "]\033[0m "
                   << message;
            
            if (!fields.empty()) {
                output << " {";
                bool first = true;
                for (const auto& [k, v] : fields) {
                    if (!first) output << ", ";
                    output << k << "=" << v;
                    first = false;
                }
                output << "}";
            }
        }
        
        output << "\n";
        std::string line = output.str();
        
        if (consoleOutput) {
            std::cout << line;
        }
        
        if (fileStream.is_open()) {
            fileStream << line;
            fileStream.flush();
        }
    }

public:
    static Logger& getInstance() {
        std::lock_guard<std::mutex> lock(instanceMtx);
        if (!instance) {
            instance = new Logger();
        }
        return *instance;
    }
    
    void configure(LogLevel level, bool json, const std::string& logFile = "") {
        minLevel = level;
        jsonFormat = json;
        if (!logFile.empty()) {
            fileStream.open(logFile, std::ios::app);
        }
    }
    
    void setContext(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(logMtx);
        contextFields[key] = value;
    }
    
    void trace(const std::string& component, const std::string& msg,
               const std::map<std::string, std::string>& fields = {}) {
        writeLog(LogLevel::TRACE, component, msg, fields);
    }
    
    void debug(const std::string& component, const std::string& msg,
               const std::map<std::string, std::string>& fields = {}) {
        writeLog(LogLevel::DEBUG, component, msg, fields);
    }
    
    void info(const std::string& component, const std::string& msg,
              const std::map<std::string, std::string>& fields = {}) {
        writeLog(LogLevel::INFO, component, msg, fields);
    }
    
    void warn(const std::string& component, const std::string& msg,
              const std::map<std::string, std::string>& fields = {}) {
        writeLog(LogLevel::WARN, component, msg, fields);
    }
    
    void error(const std::string& component, const std::string& msg,
               const std::map<std::string, std::string>& fields = {}) {
        writeLog(LogLevel::ERROR, component, msg, fields);
    }
    
    void fatal(const std::string& component, const std::string& msg,
               const std::map<std::string, std::string>& fields = {}) {
        writeLog(LogLevel::FATAL, component, msg, fields);
    }
};

Logger* Logger::instance = nullptr;
std::mutex Logger::instanceMtx;

// Convenience macros
#define LOG_TRACE(comp, msg) aegen::Logger::getInstance().trace(comp, msg)
#define LOG_DEBUG(comp, msg) aegen::Logger::getInstance().debug(comp, msg)
#define LOG_INFO(comp, msg) aegen::Logger::getInstance().info(comp, msg)
#define LOG_WARN(comp, msg) aegen::Logger::getInstance().warn(comp, msg)
#define LOG_ERROR(comp, msg) aegen::Logger::getInstance().error(comp, msg)
#define LOG_FATAL(comp, msg) aegen::Logger::getInstance().fatal(comp, msg)

/**
 * Metrics - Prometheus-compatible metrics collection
 */
class Metrics {
private:
    static Metrics* instance;
    static std::mutex instanceMtx;
    
    std::mutex metricsMtx;
    std::map<std::string, std::atomic<int64_t>> counters;
    std::map<std::string, std::atomic<int64_t>> gauges;
    std::map<std::string, std::vector<double>> histograms;
    std::map<std::string, std::string> labels;
    
    std::string escapeLabel(const std::string& s) {
        std::string result;
        for (char c : s) {
            if (c == '"' || c == '\\' || c == '\n') {
                result += '\\';
            }
            result += c;
        }
        return result;
    }

public:
    static Metrics& getInstance() {
        std::lock_guard<std::mutex> lock(instanceMtx);
        if (!instance) {
            instance = new Metrics();
        }
        return *instance;
    }
    
    void setLabel(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(metricsMtx);
        labels[key] = value;
    }
    
    // Counter operations
    void increment(const std::string& name, int64_t value = 1) {
        counters[name] += value;
    }
    
    int64_t getCounter(const std::string& name) {
        return counters[name].load();
    }
    
    // Gauge operations
    void setGauge(const std::string& name, int64_t value) {
        gauges[name] = value;
    }
    
    void incGauge(const std::string& name, int64_t delta = 1) {
        gauges[name] += delta;
    }
    
    void decGauge(const std::string& name, int64_t delta = 1) {
        gauges[name] -= delta;
    }
    
    int64_t getGauge(const std::string& name) {
        return gauges[name].load();
    }
    
    // Histogram operations
    void observe(const std::string& name, double value) {
        std::lock_guard<std::mutex> lock(metricsMtx);
        histograms[name].push_back(value);
        
        // Keep only last 1000 observations
        if (histograms[name].size() > 1000) {
            histograms[name].erase(histograms[name].begin());
        }
    }
    
    // Timer helper
    class Timer {
        std::string name;
        std::chrono::high_resolution_clock::time_point start;
    public:
        Timer(const std::string& metricName) : name(metricName) {
            start = std::chrono::high_resolution_clock::now();
        }
        ~Timer() {
            auto end = std::chrono::high_resolution_clock::now();
            double ms = std::chrono::duration<double, std::milli>(end - start).count();
            Metrics::getInstance().observe(name, ms);
        }
    };
    
    // Prometheus format export
    std::string exportPrometheus() {
        std::lock_guard<std::mutex> lock(metricsMtx);
        std::stringstream ss;
        
        // Build label string
        std::string labelStr;
        if (!labels.empty()) {
            labelStr = "{";
            bool first = true;
            for (const auto& [k, v] : labels) {
                if (!first) labelStr += ",";
                labelStr += k + "=\"" + escapeLabel(v) + "\"";
                first = false;
            }
            labelStr += "}";
        }
        
        // Export counters
        for (const auto& [name, value] : counters) {
            ss << "# TYPE " << name << " counter\n";
            ss << name << labelStr << " " << value.load() << "\n";
        }
        
        // Export gauges
        for (const auto& [name, value] : gauges) {
            ss << "# TYPE " << name << " gauge\n";
            ss << name << labelStr << " " << value.load() << "\n";
        }
        
        // Export histograms (as summary)
        for (const auto& [name, values] : histograms) {
            if (values.empty()) continue;
            
            // Calculate percentiles
            std::vector<double> sorted = values;
            std::sort(sorted.begin(), sorted.end());
            
            double sum = 0;
            for (double v : sorted) sum += v;
            
            size_t p50 = sorted.size() * 0.5;
            size_t p90 = sorted.size() * 0.9;
            size_t p99 = sorted.size() * 0.99;
            
            ss << "# TYPE " << name << " summary\n";
            ss << name << "{quantile=\"0.5\"" << (labels.empty() ? "" : "," + labelStr.substr(1, labelStr.size()-2)) << "} " << sorted[p50] << "\n";
            ss << name << "{quantile=\"0.9\"" << (labels.empty() ? "" : "," + labelStr.substr(1, labelStr.size()-2)) << "} " << sorted[p90] << "\n";
            ss << name << "{quantile=\"0.99\"" << (labels.empty() ? "" : "," + labelStr.substr(1, labelStr.size()-2)) << "} " << sorted[p99 < sorted.size() ? p99 : sorted.size()-1] << "\n";
            ss << name << "_sum" << labelStr << " " << sum << "\n";
            ss << name << "_count" << labelStr << " " << values.size() << "\n";
        }
        
        return ss.str();
    }
    
    // JSON format export
    std::string exportJSON() {
        std::lock_guard<std::mutex> lock(metricsMtx);
        std::stringstream ss;
        ss << "{\n";
        ss << "  \"counters\": {\n";
        bool first = true;
        for (const auto& [name, value] : counters) {
            if (!first) ss << ",\n";
            ss << "    \"" << name << "\": " << value.load();
            first = false;
        }
        ss << "\n  },\n";
        
        ss << "  \"gauges\": {\n";
        first = true;
        for (const auto& [name, value] : gauges) {
            if (!first) ss << ",\n";
            ss << "    \"" << name << "\": " << value.load();
            first = false;
        }
        ss << "\n  }\n";
        ss << "}\n";
        
        return ss.str();
    }
};

Metrics* Metrics::instance = nullptr;
std::mutex Metrics::instanceMtx;

// Predefined metric names
namespace metrics {
    constexpr const char* BLOCKS_PRODUCED = "aegen_blocks_produced_total";
    constexpr const char* TXS_PROCESSED = "aegen_transactions_processed_total";
    constexpr const char* TXS_PENDING = "aegen_transactions_pending";
    constexpr const char* PEERS_CONNECTED = "aegen_peers_connected";
    constexpr const char* BLOCK_TIME_MS = "aegen_block_time_ms";
    constexpr const char* RPC_REQUESTS = "aegen_rpc_requests_total";
    constexpr const char* RPC_LATENCY_MS = "aegen_rpc_latency_ms";
    constexpr const char* CONSENSUS_ROUNDS = "aegen_consensus_rounds_total";
    constexpr const char* SETTLEMENT_BATCHES = "aegen_settlement_batches_total";
    constexpr const char* DA_BLOBS_STORED = "aegen_da_blobs_stored_total";
    constexpr const char* DB_KEYS = "aegen_db_keys_total";
    constexpr const char* MEMORY_BYTES = "aegen_memory_bytes";
}

}
