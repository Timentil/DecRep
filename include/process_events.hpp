#ifndef PROCESS_EVENTS_HPP_
#define PROCESS_EVENTS_HPP_

#include <unordered_map>
#include "dec_rep_fwd.hpp"

namespace process_events {

std::string get_name(const std::string &full_path);

enum class Event {
    CONNECT,
    ADD_FILE,
    ADD_FOLDER,
    CHANGE_FILE,
    UNTRACK_FILE,
    UNTRACK_FOLDER,
    DELETE_LOCAL_FILE,
    ADD_USER,
    EXIT,
    DOWNLOAD,
    INVALID
};

const std::unordered_map<std::string, Event> events = {
    {"connect", Event::CONNECT},
    {"add_file", Event::ADD_FILE},
    {"add_folder", Event::ADD_FOLDER},
    {"change_file", Event::CHANGE_FILE},
    {"untrack_file", Event::UNTRACK_FILE},
    {"untrack_folder", Event::UNTRACK_FOLDER},
    {"delete_local_file", Event::DELETE_LOCAL_FILE},
    {"exit", Event::EXIT},
    {"download", Event::DOWNLOAD}};

Event get_event(const std::string &event);

void handle_message(std::istringstream &msg);

void connect(DecRep &app, std::istringstream &msg);

void add_file(
    DecRep &app,
    const std::string &local_file_path,
    const std::string &DecRep_path,
    const std::string &username,
    const std::string &ip
);

void add_folder(
    DecRep &app,
    const std::string &local_folder_path,
    const std::string &DecRep_path,
    const std::string &username,
    const std::string &ip
);

void add_user(DecRep &app, const std::string &username, const std::string &ip);

void update_file(
    DecRep &app,
    const std::string &local_path,
    const std::string &username,
    const std::string &ip,
    std::string &new_hash,  //??
    std::size_t &new_size   //??
);

void update_local_path(
    DecRep &app,
    const std::string &old_local_path,  // Может быть заменен
    const std::string &new_local_path,
    const std::string &username,
    const std::string &ip
);

void untrack_file(DecRep &app, const std::string &full_DecRep_path);

void untrack_folder(DecRep &app, const std::string &DecRep_path);

void delete_local_file(
    DecRep &app,
    const std::string &local_path,
    const std::string &username,
    const std::string &ip
);

void delete_user(
    DecRep &app,
    const std::string &username,
    const std::string &ip
);

bool is_db_empty(DecRep &app);
};  // namespace process_events

#endif  // PROCESS_EVENTS_HPP_