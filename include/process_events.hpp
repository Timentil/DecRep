#ifndef PROCESS_EVENTS_HPP_
#define PROCESS_EVENTS_HPP_

#include "db_manager.hpp"
#include "dec_rep_fs.hpp"
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <functional>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace beast = boost::beast;
namespace json = boost::json;
namespace http = beast::http;
using CommandHandler = std::function<bool(const std::vector<std::string_view> &)>;

namespace Events {

std::string get_name(const std::string &full_path);
std::vector<std::string_view> split_str(std::string_view str, char delimiter);

class EventHandler {
private:
    DBManager::Manager &dbManager;
    DecRepFS::FS &decRepFS;

public:
    std::unordered_map<std::string, CommandHandler> func_map;

    EventHandler(DBManager::Manager &db, DecRepFS::FS &fs);

    std::string get_db_data();
    void import_data(const std::string &json_str);

    bool add_file(const std::vector<std::string_view> &) const;
    bool add_folder(const std::vector<std::string_view> &) const;
    bool rename_DecRep_file(const std::vector<std::string_view> &) const;
    bool rename_DecRep_folder(const std::vector<std::string_view> &) const;
    bool change_DecRep_path(const std::vector<std::string_view> &) const;
    bool add_user(const std::vector<std::string_view> &) const;
    bool update_file(const std::vector<std::string_view> &) const;
    bool update_local_file_path(const std::vector<std::string_view> &) const;
    bool update_local_folder_path(const std::vector<std::string_view> &) const;
    bool untrack_file(const std::vector<std::string_view> &) const;
    bool untrack_folder(const std::vector<std::string_view> &) const;
    bool delete_local_file(const std::vector<std::string_view> &) const;
    bool delete_user(const std::vector<std::string_view> &) const;

    http::message_generator handle_request(http::request<http::string_body> &&req);
    void handle_response(http::response<http::string_body> &&res);
};
} // namespace Events

#endif // PROCESS_EVENTS_HPP_