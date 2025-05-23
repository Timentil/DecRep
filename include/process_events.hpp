#ifndef PROCESS_EVENTS_HPP_
#define PROCESS_EVENTS_HPP_

#include <unordered_map>
#include "db_manager.hpp"
#include "dec_rep_fs.hpp"

namespace Events {

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

std::string get_name(const std::string &full_path);

Event get_event(const std::string &event);

void handle_message(std::istringstream &msg);

class EventHandler {
private:
    DBManager::Manager &dbManager;
    DecRepFS::FS &decRepFS;

public:
    EventHandler(DBManager::Manager &db, DecRepFS::FS &fs);

    void connect(std::istringstream &msg);

    void add_file(
        const std::string &local_file_path,
        const std::string &DecRep_path,
        const std::string &username,
        const std::string &ip,
        const std::string &port
    );

    void add_folder(
        const std::string &local_folder_path,
        const std::string &DecRep_path,
        const std::string &username,
        const std::string &ip,
        const std::string &port
    );

    void add_user(
        const std::string &username,
        const std::string &ip,
        const std::string &port
    );

    void update_file(
        const std::string &local_path,
        const std::string &username,
        const std::string &ip,
        const std::string &port,
        std::string &new_hash,  //??
        std::size_t &new_size   //??
    );

    void update_local_path(
        const std::string &old_local_path,  // Может быть заменен
        const std::string &new_local_path,
        const std::string &username,
        const std::string &ip,
        const std::string &port
    );

    void untrack_file(const std::string &full_DecRep_path);

    void untrack_folder(const std::string &DecRep_path);

    void delete_local_file(
        const std::string &local_path,
        const std::string &username,
        const std::string &ip,
        const std::string &port
    );

    void delete_user(
        const std::string &username,
        const std::string &ip,
        const std::string &port
    );
};
};  // namespace Events

#endif  // PROCESS_EVENTS_HPP_