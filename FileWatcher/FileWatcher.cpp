#include "FileWatcher.hpp"
#include <boost/asio/post.hpp>

FileWatcher::FileWatcher(boost::asio::io_context& io, EventCallback cb)
    : io_(io)
    , callback_(std::move(cb))
    , watcher_(std::make_unique<efsw::FileWatcher>(true))
{}


FileWatcher::~FileWatcher() = default;


void FileWatcher::run() const {
    watcher_->watch();
}


void FileWatcher::addWatch(const std::string& path) {
    namespace fs = std::filesystem;
    const fs::path p{path};

    auto addDir = [&](const std::string& dir){
        if (watched_dirs.insert(dir).second) {
            watcher_->addWatch(dir, this, false);
        }
    };

    if (fs::is_directory(p)) {
        for (auto& ent : fs::recursive_directory_iterator(p)) {
            if (!ent.is_regular_file()) continue;
            std::string file = fs::absolute(ent.path()).string();
            watched_files.insert(file);
            std::string dir = fs::absolute(ent.path().parent_path()).string();
            addDir(dir);
        }
    }
    else if (fs::is_regular_file(p)) {
        const std::string file = fs::absolute(p).string();
        watched_files.insert(file);
        const std::string dir = fs::absolute(p.parent_path()).string();
        addDir(dir);
    }
}


FW_Event::Type FileWatcher::to_Event(efsw::Action action) {
    switch (action) {
        case efsw::Actions::Add:      return FW_Event::Type::Added;
        case efsw::Actions::Delete:   return FW_Event::Type::Deleted;
        case efsw::Actions::Modified: return FW_Event::Type::Modified;
        case efsw::Actions::Moved:    return FW_Event::Type::Moved;
    }
    return FW_Event::Type::Modified;
}


void FileWatcher::handleFileAction(
    efsw::WatchID,
    const std::string& dir,
    const std::string& filename,
    const efsw::Action action,
    const std::string oldFilename
) {
    namespace fs = std::filesystem;

    const std::string oldFull =
        fs::absolute(fs::path(dir) / oldFilename).string();
    const std::string newFull = fs::absolute(fs::path(dir) / filename).string();

    FW_Event ev;
    ev.type     = to_Event(action);
    ev.new_path = newFull;

    if (action == efsw::Actions::Moved) {
        if (watched_files.erase(oldFull)) {
            ev.old_path = oldFull;
            watched_files.insert(newFull);
        } else {
            return;
        }
    }
    else if (action == efsw::Actions::Delete) {
        if (!watched_files.erase(newFull)) return;
    }
    else if (action == efsw::Actions::Modified) {
        if (!watched_files.contains(newFull)) return;
    }
    else if (action == efsw::Actions::Add) {
        if (!watched_files.contains(newFull)) return;
    }

    boost::asio::post(io_, [cb = callback_, ev = std::move(ev)](){
        cb(ev);
    });
}

