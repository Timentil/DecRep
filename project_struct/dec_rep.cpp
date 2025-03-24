#include <unordered_map>
#include "db_manager.cpp"
#include "process_events.cpp"
// куча инклудов

enum class Event {
    ADD_FILE,
    ADD_FOLDER,
    CHANGE_FILE,  // к этому методу у пользователя не должно быть доступа
    // CHANGE_FILE_NAME,
    UNTRACK_FILE,
    UNTRACK_FOLDER,
    DELETE_LOCAL_FILE,  // и к этому
    ADD_USER,           // и к этому
    EXIT,
    DOWNLOAD,
    INVALID
    // ???
};

const std::unordered_map<std::string, Event> events = {
    {"add_file", Event::ADD_FILE},
    {"add_folder", Event::ADD_FOLDER},
    {"change_file", Event::CHANGE_FILE},
    {"untrack_file", Event::UNTRACK_FILE},
    {"untrack_folder", Event::UNTRACK_FOLDER},
    {"delete_local_file", Event::DELETE_LOCAL_FILE},
    {"exit", Event::EXIT},
    {"download", Event::DOWNLOAD}};

Event getEvent(const std::string &event) {
    if (const auto it = events.find(event); it != events.end()) {
        return it->second;
    }
    return Event::INVALID;
}

class DecRec {
    DecRep() : { /* init other modules */
    }

    // file_watcher, HTTP server - "run-time" classes
    // dec_rep_fs init - construct form database
};

int main() {
    DecRep d;
    d.fs

        while () {
        std::getline();
    }

    while () {
        // HTTP server -> Event x;
        switch (event_id) {
            case Event::ADD:
                DecRep::add();
                break;
            case Event::DEL:
                DecRep::del();
                break;
            case Event::DOWNLOAD:
                DecRep::download();
            case Event::EXIT:
                decRep::exit();
            case Event::INVALID:
                DecRep::invalid();
        }
    }
}