#ifndef FILEWATCHER_HPP
#define FILEWATCHER_HPP

#include "change_propagator.hpp"
#include <boost/asio.hpp>
#include <efsw/efsw.hpp>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <unordered_set>

struct FW_Event {
    enum class Type { Added,
                      Deleted,
                      Modified,
                      Moved };
    Type type;
    std::vector<std::string> old_paths; // все старые пути
    std::vector<std::string> new_paths; // все новые пути
};

class FileWatcher final : public efsw::FileWatchListener {
    using EventCallback = std::function<void(const FW_Event &)>;

private:
    static FW_Event::Type to_Event(efsw::Action action);

    ChangePropagator::ChangePropagator &prop_;

    boost::asio::io_context &io_;
    EventCallback callback_;
    std::unique_ptr<efsw::FileWatcher> watcher_;

    std::unordered_set<std::string> watched_files; // файлы, за которыми следим
    std::unordered_set<std::string> watched_dirs; // директории с watch

public:
    FileWatcher(ChangePropagator::ChangePropagator &prop, boost::asio::io_context &io, EventCallback cb);
    ~FileWatcher() override;

    // блокирующий
    void run() const;

    void addWatch(const std::string &path);

    // главный метод
    void handleFileAction(
        efsw::WatchID watch_id,
        const std::string &dir,
        const std::string &filename,
        efsw::Action action,
        std::string oldFilename
    ) override;
};

#endif // FILEWATCHER_HPP
