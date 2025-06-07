#ifndef PROCESS_EVENTS_HPP_
#define PROCESS_EVENTS_HPP_

#include "db_manager.hpp"
#include "dec_rep_fs.hpp"
#include <boost/beast/http.hpp>
#include <boost/json/src.hpp>
#include <functional>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace beast = boost::beast;
namespace json = boost::json;
namespace http = beast::http;
using CommandHandler = std::function<void(const std::vector<std::string_view> &)>;

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

    void help([[maybe_unused]] const std::vector<std::string_view> &);
    void add_file(const std::vector<std::string_view> &);
    void add_folder(const std::vector<std::string_view> &);
    void add_user(const std::vector<std::string_view> &);
    void update_file(const std::vector<std::string_view> &);
    void update_local_path(const std::vector<std::string_view> &);
    void untrack_file(const std::vector<std::string_view> &);
    void untrack_folder(const std::vector<std::string_view> &);
    void delete_local_file(const std::vector<std::string_view> &);
    void delete_user(const std::vector<std::string_view> &);

    http::message_generator handle_request(http::request<http::string_body> &&req);
    void handle_response(http::response<http::string_body> &&res);
};
}; // namespace Events

#endif // PROCESS_EVENTS_HPP_