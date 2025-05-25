#ifndef PROCESS_EVENTS_HPP_
#define PROCESS_EVENTS_HPP_

#include "db_manager.hpp"
#include "dec_rep_fs.hpp"
#include <boost/json/src.hpp>
#include <functional>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace json = boost::json;

namespace Events {

std::string get_name(const std::string &full_path);

std::vector<std::string_view> split_str(std::string_view str);

class EventHandler {
private:
    DBManager::Manager &dbManager;
    DecRepFS::FS &decRepFS;

public:
    std::unordered_map<
        std::string,
        std::function<bool(const std::vector<std::string_view> &)>>
        func_map;

    EventHandler(DBManager::Manager &db, DecRepFS::FS &fs);

    std::string get_db_data();

    void import_data(const std::string &json_str);

    // local_file_path,
    // DecRep_path,
    // username,
    // ip,
    // port
    bool add_file(const std::vector<std::string_view> &);

    // local_folder_path,
    // DecRep_path,
    // username,
    // ip,
    // port
    bool add_folder(const std::vector<std::string_view> &);

    // username,
    // ip,
    // port
    bool add_user(const std::vector<std::string_view> &);

    // local_path,
    // username,
    // new_hash,  //??
    // new_size,   //??
    // ip,
    // port
    bool update_file(const std::vector<std::string_view> &);

    // old_local_path,  // Может быть заменен
    // new_local_path,
    // username,
    // ip,
    // port
    bool update_local_path(const std::vector<std::string_view> &);

    // full_DecRep_path,
    // ip,
    // port
    bool untrack_file(const std::vector<std::string_view> &);

    // DecRep_path,
    // ip,
    // port
    bool untrack_folder(const std::vector<std::string_view> &);

    // local_path,
    // username,
    // ip,
    // port
    bool delete_local_file(const std::vector<std::string_view> &);

    // username,
    // ip,
    // port
    bool delete_user(const std::vector<std::string_view> &);
};
}; // namespace Events

#endif // PROCESS_EVENTS_HPP_