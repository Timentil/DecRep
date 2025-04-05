#include "dec_rep.hpp"

namespace process_events {

std::string get_name(const std::string &full_path) {
    fs::path p(full_path);
    while (!p.empty() && p.filename().empty()) {
        p = p.parent_path();
    }
    return p.filename().string();
}

Event get_event(const std::string &event) {
    if (const auto it = events.find(event); it != events.end()) {
        return it->second;
    }
    return Event::INVALID;
}

void handle_message(std::istringstream &msg) {
    std::string event{};
    msg >> event;
    Event event_id = get_event(event);
    switch (event_id) {
        case Event::CONNECT:
            // connect(msg);
            break;
        case Event::ADD_FILE:
            /* process event code*/
            break;
        case Event::ADD_FOLDER:
            /* process event code*/
            break;
        case Event::UNTRACK_FILE:
            /* process event code*/
            break;
        case Event::UNTRACK_FOLDER:
            /* process event code*/
            break;
        case Event::EXIT:
            /* process event code*/
            break;
        case Event::DOWNLOAD:
            /* process event code*/
            break;
        case Event::INVALID:
            /* process event code*/
            break;
    }
}

void connect(DecRep &app, std::istringstream &msg) {
}

void add_file(
    DecRep &app,
    const std::string &local_file_path,
    const std::string &DecRep_path,
    const std::string &username,
    const std::string &ip
) {
    const std::string file_name = get_name(local_file_path);

    app.dbManager.add_file(
        local_file_path, file_name, DecRep_path, username, ip
    );
    app.decRepFS.add_file(DecRep_path, file_name);
}

void add_folder(
    DecRep &app,
    const std::string &local_folder_path,
    const std::string &DecRep_path,
    const std::string &username,
    const std::string &ip
) {
    const std::string folder_name = get_name(local_folder_path);
    app.dbManager.add_folder(local_folder_path, DecRep_path, username, ip);
    app.decRepFS.add_folder(DecRep_path, local_folder_path);
}

void add_user(DecRep &app, const std::string &username, const std::string &ip) {
    app.dbManager.add_into_Users(username, ip);
}

void update_file(
    DecRep &app,
    const std::string &local_path,
    const std::string &username,
    const std::string &ip,
    std::string &new_hash,
    std::size_t &new_size
) {
    app.dbManager.update_file(local_path, username, ip, new_hash, new_size);
}

void update_local_path(
    DecRep &app,
    const std::string &old_local_path,
    const std::string &new_local_path,
    const std::string &username,
    const std::string &ip
) {
    app.dbManager.update_local_path(
        old_local_path, new_local_path, username, ip
    );
}

void untrack_file(DecRep &app, const std::string &full_DecRep_path) {
    app.dbManager.untrack_file(full_DecRep_path);
    app.decRepFS.delete_file(full_DecRep_path);
}

void untrack_folder(DecRep &app, const std::string &DecRep_path) {
    app.dbManager.untrack_folder(DecRep_path);
    app.decRepFS.delete_folder(DecRep_path);
}

void delete_local_file(
    DecRep &app,
    const std::string &local_path,
    const std::string &username,
    const std::string &ip
) {
    const std::string delete_res =
        app.dbManager.delete_local_file(local_path, username, ip);
    if (!delete_res.empty()) {
        app.decRepFS.delete_file(delete_res);
    }
}

void delete_user(
    DecRep &app,
    const std::string &username,
    const std::string &ip
) {
    const std::vector<std::string> deleted_files =
        app.dbManager.delete_user(username, ip);
    app.decRepFS.delete_user_files(deleted_files);
}

bool is_db_empty(DecRep &app) {
    return app.dbManager.is_users_empty();
}
}  // namespace process_events