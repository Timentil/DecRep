#include "file_watcher.hpp"
#include <boost/asio/post.hpp>

FileWatcher::FileWatcher(ChangePropagator::ChangePropagator &prop, boost::asio::io_context &io, EventCallback cb)
    : prop_(prop)
    , io_(io)
    , callback_(std::move(cb))
#if defined(_WIN32) || defined(_WIN64)
    , watcher_(std::make_unique<efsw::FileWatcher>(false))
#else
    , watcher_(std::make_unique<efsw::FileWatcher>(true))
#endif
{ // generic
}

FileWatcher::~FileWatcher() = default;

void FileWatcher::run() const
{
    watcher_->watch();
}

void FileWatcher::addWatch(const std::string &path)
{
    namespace fs = std::filesystem;
    const fs::path p { path };

    auto addDir = [&](const std::string &dir) {
        if (watched_dirs.insert(dir).second) {
            watcher_->addWatch(dir, this, false);
        }
    };

    if (fs::is_directory(p)) {
        addDir(fs::absolute(p).string());

        if (p.has_parent_path()) {
            addDir(fs::absolute(p.parent_path()).string());
        }

        for (auto &ent : fs::recursive_directory_iterator(p)) {
            const auto absPath = fs::absolute(ent.path()).string();
            if (ent.is_directory()) {
                addDir(absPath);
            } else if (ent.is_regular_file()) {
                watched_files.insert(absPath);
            }
        }
    } else if (fs::is_regular_file(p)) {
        const std::string file = fs::absolute(p).string();
        watched_files.insert(file);
        const std::string dir = fs::absolute(p.parent_path()).string();
        addDir(dir);
    }
}

FW_Event::Type FileWatcher::to_Event(const efsw::Action action)
{
    switch (action) {
    case efsw::Actions::Add:
        return FW_Event::Type::Added;
    case efsw::Actions::Delete:
        return FW_Event::Type::Deleted;
    case efsw::Actions::Modified:
        return FW_Event::Type::Modified;
    case efsw::Actions::Moved:
        return FW_Event::Type::Moved;
    }
    return FW_Event::Type::Modified;
}

void FileWatcher::handleFileAction(
    efsw::WatchID,
    const std::string &dir,
    const std::string &filename,
    const efsw::Action action,
    const std::string oldFilename
)
{
    namespace fs = std::filesystem;

    const std::string oldFull = fs::absolute(fs::path(dir) / oldFilename).string();
    const std::string newFull = fs::absolute(fs::path(dir) / filename).string();

    FW_Event event { to_Event(action), {}, {} };

    if (action == efsw::Actions::Moved) {
        if (watched_files.contains(oldFull)) {
            event.old_paths.push_back("\"" + oldFull + "\"");
            event.new_paths.push_back("\"" + newFull + "\"");
            watched_files.erase(oldFull);
            watched_files.insert(newFull);
        } else if (watched_dirs.contains(oldFull)) {

            const char sep = static_cast<char>(fs::path::preferred_separator);
            const std::string oldPref = oldFull + std::string(1, sep);
            const std::string newPref = newFull + std::string(1, sep);
            // const std::string oldPref = oldFull + fs::path::preferred_separator;
            // const std::string newPref = newFull + fs::path::preferred_separator;

            std::vector<std::string> dirsToRemove;
            dirsToRemove.push_back(oldFull);
            for (auto &d : watched_dirs) {
                if (d.rfind(oldPref, 0) == 0) {
                    dirsToRemove.push_back(d);
                }
            }

            std::vector<std::string> filesToRemove;
            for (auto &f : watched_files) {
                if (f.rfind(oldPref, 0) == 0) {
                    filesToRemove.push_back(f);
                }
            }

            for (auto &oldD : dirsToRemove) {
                std::string newD = (oldD == oldFull ? newFull : newPref + oldD.substr(oldPref.size()));
                watched_dirs.erase(oldD);
                watched_dirs.insert(newD);
            }
            for (auto &oldF : filesToRemove) {
                std::string newF = newPref + oldF.substr(oldPref.size());
                watched_files.erase(oldF);
                watched_files.insert(newF);
                event.old_paths.push_back("\"" + oldF + "\"");
                event.new_paths.push_back("\"" + newF + "\"");
            }
        } else {
            return;
        }
    } else if (action == efsw::Actions::Delete) {
        if (!watched_files.contains(newFull)) {
            return;
        }
        event.old_paths.push_back("\"" + newFull + "\"");
        watched_files.erase(newFull);
    } else if (action == efsw::Actions::Modified) {
        if (!watched_files.contains(newFull)) {
            return;
        }
        event.new_paths.push_back("\"" + newFull + "\"");
    } else {
        return; // игнор Add
    }

    boost::asio::post(io_, [this, ev = std::move(event)]() mutable {
        if (ev.type == FW_Event::Type::Deleted) {
            for (auto &oldp : ev.old_paths) {
                std::vector<std::string_view> parts { "delete_local_file", oldp };
                prop_.on_local_change(parts);
            }
        } else if (ev.type == FW_Event::Type::Modified) {
            for (auto &newp : ev.new_paths) {
                std::vector<std::string_view> parts { "update_file", newp };
                prop_.on_local_change(parts);
            }
        } else if (ev.type == FW_Event::Type::Moved) {
            for (size_t i = 0; i < ev.old_paths.size(); ++i) {
                std::vector<std::string_view> parts {
                    "update_local_path",
                    ev.old_paths[i],
                    ev.new_paths[i]
                };
            prop_.on_local_change(parts);
            }
        }
    });
}
