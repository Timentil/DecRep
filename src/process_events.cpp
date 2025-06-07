#include "process_events.hpp"

namespace fs = std::filesystem;

namespace Events {

std::string get_name(const std::string &full_path)
{
    fs::path p(full_path);
    while (!p.empty() && p.filename().empty()) {
        p = p.parent_path();
    }
    return p.filename().string();
}

std::vector<std::string_view> split_str(std::string_view str, char delimiter)
{
    std::vector<std::string_view> result;
    while (true) {
        size_t pos_delim = str.find(delimiter);
        size_t pos_quote = str.find('\"');
        if (pos_quote < pos_delim) {
            str.remove_prefix(pos_quote + 1);
            pos_quote = str.find('\"');
            result.push_back(str.substr(0, pos_quote));
            str.remove_prefix(pos_quote + 1);
            continue;
        }

        if (str.substr(0, pos_delim) != "") {
            result.push_back(str.substr(0, pos_delim));
        }

        if (pos_delim == str.npos) {
            break;
        } else {
            str.remove_prefix(pos_delim + 1);
        }
    }

    return result;
}

EventHandler::EventHandler(DBManager::Manager &db, DecRepFS::FS &fs)
    : dbManager(db)
    , decRepFS(fs)
{
    func_map = {
        { "add_file", [this](const auto &params) { return this->add_file(params); } },
        { "add_folder", [this](const auto &params) { return this->add_folder(params); } },
        { "change_file", [this](const auto &params) { return this->update_file(params); } },
        { "untrack_file", [this](const auto &params) { return this->untrack_file(params); } },
        { "untrack_folder", [this](const auto &params) { return this->untrack_folder(params); } },
        { "delete_local_file", [this](const auto &params) { return this->delete_local_file(params); } }
    };
};

std::string EventHandler::get_db_data()
{
    return json::serialize(dbManager.get_all_data());
}

void EventHandler::import_data(const std::string &json_str)
{
    json::value jv = json::parse(json_str);
    json::object root = jv.as_object();

    // Users
    for (const auto &user_val : root["users"].as_array()) {
        json::object user = user_val.as_object();
        dbManager.insert_into_Users(
            user["username"].as_string().c_str(),
            user["ip"].as_string().c_str(),
            user["port"].as_string().c_str(),
            user["first_connection_time"].as_string().c_str()
        );
    }

    // Files
    for (const auto &file_val : root["files"].as_array()) {
        json::object file = file_val.as_object();
        dbManager.insert_into_Files(
            file["file_name"].as_string().c_str(),
            file["file_size"].as_string().c_str(),
            file["file_hash"].as_string().c_str(),
            file["addition_time"].as_string().c_str(),
            file["last_modified"].as_string().c_str(),
            file["DecRep_path"].as_string().c_str(),
            file["author_id"].as_string().c_str()
        );
    }

    // FileOwners
    for (const auto &fo_val : root["file_owners"].as_array()) {
        json::object fo = fo_val.as_object();
        dbManager.insert_into_FileOwners(
            fo["owner_id"].as_string().c_str(),
            fo["file_id"].as_string().c_str(),
            fo["local_path"].as_string().c_str()
        );
    }
}

bool EventHandler::add_file(const std::vector<std::string_view> &params)
{
    if (params.size() != 5) {
        std::cout << "Invalid args count:" << params.size() << '\n';
        return EXIT_FAILURE;
    }

    const std::string local_file_path(params[0]);
    const std::string DecRep_path(params[1]);
    const std::string username(params[2]);
    const std::string ip(params[3]);
    const std::string port(params[4]);
    const std::string file_name = get_name(local_file_path);

    dbManager.add_file(
        local_file_path, file_name, DecRep_path, username, ip, port
    );
    decRepFS.add_file(DecRep_path, file_name);

    return EXIT_SUCCESS;
}

bool EventHandler::add_folder(const std::vector<std::string_view> &params)
{
    if (params.size() != 5) {
        std::cout << "Invalid args count:" << params.size() << '\n';
        return EXIT_FAILURE;
    }

    const std::string local_folder_path(params[0]);
    const std::string DecRep_path(params[1]);
    const std::string username(params[2]);
    const std::string ip(params[3]);
    const std::string port(params[4]);
    const std::string folder_name = get_name(local_folder_path);

    dbManager.add_folder(local_folder_path, DecRep_path, username, ip, port);
    decRepFS.add_folder(DecRep_path, local_folder_path);

    return EXIT_SUCCESS;
}

bool EventHandler::add_user(const std::vector<std::string_view> &params)
{
    if (params.size() != 3) {
        std::cout << "Invalid args count:" << params.size() << '\n';
        return EXIT_FAILURE;
    }

    const std::string username(params[0]);
    const std::string ip(params[1]);
    const std::string port(params[2]);

    dbManager.insert_into_Users(username, ip, port);

    return EXIT_SUCCESS;
}

bool EventHandler::update_file(const std::vector<std::string_view> &params)
{
    if (params.size() != 6) {
        std::cout << "Invalid args count:" << params.size() << '\n';
        return EXIT_FAILURE;
    }

    const std::string local_path(params[0]);
    const std::string username(params[1]);
    std::string new_hash(params[2]);
    std::size_t new_size(std::stoi(std::string(params[3])));
    const std::string ip(params[4]);
    const std::string port(params[5]);

    dbManager.update_file(local_path, username, ip, port, new_hash, new_size);

    return EXIT_SUCCESS;
}

bool EventHandler::update_local_path(const std::vector<std::string_view> &params
)
{
    if (params.size() != 5) {
        std::cout << "Invalid args count:" << params.size() << '\n';
        return EXIT_FAILURE;
    }

    const std::string old_local_path(params[0]);
    const std::string new_local_path(params[1]);
    const std::string username(params[2]);
    const std::string ip(params[3]);
    const std::string port(params[4]);

    dbManager.update_local_path(
        old_local_path, new_local_path, username, ip, port
    );

    return EXIT_SUCCESS;
}

bool EventHandler::untrack_file(const std::vector<std::string_view> &params)
{
    if (params.size() != 3) {
        std::cout << "Invalid args count:" << params.size() << '\n';
        return EXIT_FAILURE;
    }

    const std::string full_DecRep_path(params[0]);

    dbManager.untrack_file(full_DecRep_path);
    decRepFS.delete_file(full_DecRep_path);

    return EXIT_SUCCESS;
}

bool EventHandler::untrack_folder(const std::vector<std::string_view> &params)
{
    if (params.size() != 3) {
        std::cout << "Invalid args count:" << params.size() << '\n';
        return EXIT_FAILURE;
    }

    const std::string DecRep_path(params[0]);

    dbManager.untrack_folder(DecRep_path);
    decRepFS.delete_folder(DecRep_path);
    
    return EXIT_SUCCESS;
}

bool EventHandler::delete_local_file(const std::vector<std::string_view> &params
)
{
    if (params.size() != 4) {
        std::cout << "Invalid args count:" << params.size() << '\n';
        return EXIT_FAILURE;
    }

    const std::string local_path(params[0]);
    const std::string username(params[1]);
    const std::string ip(params[2]);
    const std::string port(params[3]);

    const std::string delete_res = dbManager.delete_local_file(local_path, username, ip, port);
    if (!delete_res.empty()) {
        decRepFS.delete_file(delete_res);
    }

    return EXIT_SUCCESS;
}

bool EventHandler::delete_user(const std::vector<std::string_view> &params)
{
    if (params.size() != 3) {
        std::cout << "Invalid args count:" << params.size() << '\n';
        return EXIT_FAILURE;
    }

    const std::string username(params[0]);
    const std::string ip(params[1]);
    const std::string port(params[2]);

    const std::vector<std::string> deleted_files = dbManager.delete_user(username, ip, port);
    decRepFS.delete_user_files(deleted_files);

    return EXIT_SUCCESS;
}

http::message_generator EventHandler::handle_request(http::request<http::string_body> &&req)
{
    // Returns a simple response
    const auto response = [&req](http::status status, std::string_view msg = "") {
        http::response<http::string_body> res { status, req.version() };
        res.keep_alive(req.keep_alive());
        if (msg != "") {
            res.body() = std::string(msg);
        }
        res.prepare_payload();
        return res;
    };

    // Make sure we can handle the method
    if (req.method() != http::verb::get) {
        return response(http::status::bad_request, "Unknown HTTP-method");
    }

    std::vector<std::string_view> parts = split_str(req.target(), '/');
    std::string namespace_name(parts[0]);
    std::string event_name(parts[1]);

    // For now we can handle only "events"
    if (namespace_name != "events") {
        return response(http::status::bad_request, "Unknown namespace_name");
    }

    std::vector<std::string_view> event_args;
    if (parts.size() > 2) {
        event_args.assign(parts.begin() + 1, parts.end());
    }

    // Perfome an event
    // Сохраняется инвариант: отправили на другие устройства, значит данные корректны
    func_map[event_name](event_args);

    return response(http::status::accepted);
}

void handle_response(http::response<http::string_body> &&res)
{
    if (res.result() != http::status::accepted) {
        std::cout << res.body() << '\n';
    }
}

} // namespace Events